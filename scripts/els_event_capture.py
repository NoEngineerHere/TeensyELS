#!/usr/bin/env python3
"""Receive-only, opt-in ELS progress recorder (Python 3 + pyserial).

Example: python scripts/els_event_capture.py --port /dev/serial/by-id/... \
    --expected-source REV --output capture --duration 60
No discovery, writes, reset, upload, watchdog or commands. Opening a serial port
CAN change DTR/RTS (including pyserial/OS defaults); this is not a no-reset
hardware guarantee. Disconnects erase identity proof. A valid sample establishes
only its own claimed source, not cryptographic identity. Samples are not atomic.

Raw bytes and timestamped JSONL events flush on every write. Each stream retains
at most --files files of --max-bytes bytes (active plus .1, .2, ...); older data
is deliberately deleted. fsync runs at --fsync-interval and on rotation/close;
flushed data since the last fsync may be lost in a host crash. Use a dedicated
output directory: existing logs are appended/rotated, not a fresh-session erase.
Numeric archives outside new bounds and preexisting oversized files are deleted.
Disk failures are fatal, printed to stderr, never a successful summary. Exit 0
means valid samples and no observed faults; 1 means any fault (even restored),
missing heartbeat/progress, invalid input or no valid sample; 2 is an I/O/setup
failure. Ctrl-C preserves logs and follows the same health policy.

Missing progress is observational: host scheduling/USB gaps cannot prove a
firmware hang. Frame-only stalls suggest a display-call path, not a failed
screen. Uptime wraps are sanity-checked against host elapsed time; reset
candidates never identify power cause. A reset near rollover may be ambiguous.
"""
import argparse
from datetime import datetime, timezone
import json
import math
import os
from pathlib import Path
import sys
import time

UINT32 = 1 << 32
LEGACY = 'ELS_LEGACY_TEST_20261006_C'
LEGACY_SOURCE = '664230e3c0d5208500cadf86273b61602a8a2c97d698661c66045c7c014e06a5'
READ_LIMIT = 4096
POLL = 0.05


def parse_sample(raw):
    try:
        words = raw.decode('ascii').split()
    except UnicodeDecodeError as exc:
        raise ValueError('non_ascii') from exc
    if not words or words[0] not in ('ELS_PROGRESS', LEGACY):
        raise ValueError('unknown_schema')
    fields = {}
    for word in words[1:]:
        if '=' not in word:
            raise ValueError('malformed_keyvalue')
        key, value = word.split('=', 1)
        if not key or not value or key in fields:
            raise ValueError('duplicate_or_empty_field')
        fields[key] = value
    legacy = words[0] == LEGACY
    names = ('ms', 'fg', 'timer_done', 'frames', 'crash') if legacy else (
        'us', 'loop', 'timer', 'frames', 'oled_fault', 'ready')
    if 'source' not in fields or any(name not in fields for name in names):
        raise ValueError('missing_field')
    if len(fields['source']) > 128:
        raise ValueError('oversize_source')
    numbers = {}
    for name in names:
        text = fields[name]
        if not text.isascii() or not text.isdecimal() or len(text) > 10:
            raise ValueError('invalid_uint32')
        numbers[name] = int(text)
        if numbers[name] >= UINT32:
            raise ValueError('invalid_uint32')
    for name in ('crash',) if legacy else ('oled_fault', 'ready'):
        if numbers[name] not in (0, 1):
            raise ValueError('invalid_boolean')
    if legacy:
        if fields.get('motion') != 'INHIBITED' or fields.get('mode') != '0':
            raise ValueError('invalid_legacy_safety_fields')
        mask = fields.get('gpio_outputs', '')
        try:
            mask_value = int(mask, 16 if mask.lower().startswith('0x') else 10)
        except ValueError as exc:
            raise ValueError('invalid_gpio_mask') from exc
        if not 0 <= mask_value < UINT32:
            raise ValueError('invalid_gpio_mask')
        if fields['source'] != LEGACY_SOURCE:
            raise ValueError('legacy_source_mismatch')
    return dict(schema=words[0], source=fields['source'],
                uptime=numbers['ms' if legacy else 'us'],
                scale=0.001 if legacy else 0.000001,
                loop=numbers['fg' if legacy else 'loop'],
                timer=numbers['timer_done' if legacy else 'timer'],
                frames=numbers['frames'], ready=True if legacy else bool(numbers['ready']),
                fault=bool(numbers['crash' if legacy else 'oled_fault']))


class RotatingFile:
    def __init__(self, path, max_bytes, files):
        self.path, self.limit, self.files = Path(path), max_bytes, files
        # Apply changed retention settings to prior numeric archives as well.
        for archive in self.path.parent.glob(self.path.name + '.*'):
            suffix = archive.name[len(self.path.name) + 1:]
            if suffix.isdecimal() and (int(suffix) >= files or archive.stat().st_size > max_bytes):
                archive.unlink()
        self.handle = open(self.path, 'ab')
        self.size = self.handle.tell()
        if self.size > self.limit:
            self.handle.close()
            self.path.unlink()
            self.handle = open(self.path, 'ab')
            self.size = 0

    def rotate(self):
        self.handle.flush()
        os.fsync(self.handle.fileno())
        self.handle.close()
        if self.files == 1:
            self.path.unlink(missing_ok=True)
        else:
            Path(str(self.path) + '.' + str(self.files - 1)).unlink(missing_ok=True)
            for index in range(self.files - 2, 0, -1):
                old = Path(str(self.path) + '.' + str(index))
                if old.exists():
                    old.replace(str(self.path) + '.' + str(index + 1))
            self.path.replace(str(self.path) + '.1')
        self.handle = open(self.path, 'ab')
        self.size = 0

    def write(self, data, record=False):
        if record and len(data) > self.limit:
            raise OSError('JSON event exceeds retention file limit')
        if record and self.size + len(data) > self.limit:
            self.rotate()
        while data:
            if self.size == self.limit:
                self.rotate()
            count = min(len(data), self.limit - self.size)
            written = self.handle.write(data[:count])
            if written != count:
                raise OSError('short log write')
            self.size += count
            data = data[count:]
        self.handle.flush()

    def sync(self):
        self.handle.flush()
        os.fsync(self.handle.fileno())

    def close(self):
        if not self.handle.closed:
            try:
                self.sync()
            finally:
                self.handle.close()


class Logs:
    def __init__(self, directory, max_bytes, files, fsync_interval):
        Path(directory).mkdir(parents=True, exist_ok=True)
        self.binary = RotatingFile(Path(directory) / 'raw.bin', max_bytes, files)
        try:
            self.events = RotatingFile(Path(directory) / 'events.jsonl', max_bytes, files)
        except BaseException:
            self.binary.close()
            raise
        self.interval, self.last_sync = fsync_interval, time.monotonic()

    def raw(self, data):
        self.binary.write(data)

    def event(self, now, kind, **details):
        record = dict(monotonic=now, utc=datetime.now(timezone.utc).isoformat(),
                      event=kind, **details)
        self.events.write((json.dumps(record, separators=(',', ':')) + '\n').encode(), record=True)

    def sync_due(self, now):
        if now - self.last_sync >= self.interval:
            self.binary.sync()
            self.events.sync()
            self.last_sync = now

    def close(self):
        try:
            self.binary.close()
        finally:
            self.events.close()


class Monitor:
    def __init__(self, emit, expected_source, stall, grace):
        self.emit, self.expected = emit, expected_source
        self.stall, self.grace = stall, grace
        self.failed, self.samples = False, 0
        self.ready_samples = 0
        self.invalid_reason = None
        self.states = {}
        self.connected_at = None
        self.last = self.last_at = self.ready_at = None
        self.changes = {}
        self.last_poll = None

    def state(self, now, name, active, **details):
        if self.states.get(name, False) != active:
            self.states[name] = active
            self.emit(now, name if active else name + '_restored', **details)
        if active:
            self.failed = True

    def invalidate(self, now, reason):
        if self.last is not None:
            self.emit(now, 'identity_lost', reason=reason)
        self.last = self.last_at = self.ready_at = None
        self.changes = {}

    def connect(self, now):
        self.invalidate(now, 'new_connection')
        self.connected_at = now
        self.emit(now, 'connected', identity='unverified')

    def disconnect(self, now, reason):
        self.invalidate(now, reason)
        self.connected_at = None
        self.state(now, 'disconnected', True, reason=reason)

    def invalid(self, now, reason):
        self.invalidate(now, reason)
        if self.invalid_reason != reason:
            self.emit(now, 'input_rejected', reason=reason)
            self.invalid_reason = reason
        self.state(now, 'invalid_input', True, reason=reason)

    def sample(self, now, sample):
        if sample['source'] != self.expected:
            self.invalidate(now, 'source_mismatch')
            self.state(now, 'source_mismatch', True, observed=sample['source'])
            return
        for name in ('source_mismatch', 'invalid_input', 'disconnected', 'heartbeat_missing'):
            self.state(now, name, False)
        self.samples += 1
        self.invalid_reason = None
        if sample['ready']:
            self.ready_samples += 1
        old = self.last
        if old is None:
            self.emit(now, 'identity_verified', source=sample['source'], schema=sample['schema'])
        elif old['schema'] != sample['schema']:
            self.emit(now, 'schema_changed', schema=sample['schema'])
            old = None
        reset = False
        if old is not None:
            elapsed = max(0, now - self.last_at)
            if sample['uptime'] < old['uptime']:
                delta = (sample['uptime'] - old['uptime']) % UINT32
                wrap = delta * sample['scale'] <= elapsed + 2.0
                self.emit(now, 'uptime_wrap_candidate' if wrap else 'reset_candidate',
                          host_elapsed=elapsed, modular_elapsed=delta * sample['scale'],
                          cause='unknown; rollover/reset may be ambiguous')
                reset = not wrap
            for key in ('loop', 'timer', 'frames'):
                delta = (sample[key] - old[key]) % UINT32
                if delta >= UINT32 // 2:
                    self.emit(now, 'counter_reset_candidate', counter=key)
                    reset = True
                elif sample[key] < old[key]:
                    self.emit(now, 'counter_wrap', counter=key)
        if reset:
            self.failed = True
            old = None
            self.ready_at = None
        self.state(now, 'firmware_fault', sample['fault'])
        if not sample['ready']:
            self.ready_at = None
            self.changes = {}
            for key in ('loop', 'timer', 'frames'):
                self.state(now, key + '_stalled', False, reason='not_ready')
        else:
            if self.ready_at is None:
                self.ready_at = now
                self.changes = {key: now for key in ('loop', 'timer', 'frames')}
            for key in ('loop', 'timer', 'frames'):
                if old is None or sample[key] != old[key]:
                    self.changes[key] = now
                    self.state(now, key + '_stalled', False)
        self.last, self.last_at = sample, now
        self.emit(now, 'sample', **sample)

    def poll(self, now):
        if self.last_poll is not None:
            self.state(now, 'host_poll_gap', now - self.last_poll > self.stall,
                       ambiguity='host/USB gaps do not prove firmware hang')
        self.last_poll = now
        if self.connected_at is None:
            return
        base = self.last_at if self.last_at is not None else self.connected_at
        missing = now - base >= self.stall and now - self.connected_at >= self.grace
        self.state(now, 'heartbeat_missing', missing,
                   ambiguity='no valid heartbeat; host/USB/firmware cause unknown')
        self.state(now, 'initialization_not_ready', self.last is not None and
                   not self.last['ready'] and now - self.connected_at >= self.grace)
        if self.ready_at is not None and now - self.ready_at >= self.grace:
            for key, changed in self.changes.items():
                self.state(now, key + '_stalled', now - changed >= self.stall,
                           ambiguity=('possible display-call path, not screen failure' if key == 'frames'
                                      else 'host/USB gaps versus firmware hang unknown'))


class Framer:
    def __init__(self, limit, on_line, on_invalid):
        self.limit, self.on_line, self.on_invalid = limit, on_line, on_invalid
        self.buffer = bytearray()
        self.dropping = False

    def feed(self, data, now):
        for byte in data:
            if byte == 10:
                if not self.dropping:
                    self.on_line(now, bytes(self.buffer).rstrip(b'\r'))
                self.buffer.clear()
                self.dropping = False
            elif not self.dropping:
                if len(self.buffer) == self.limit:
                    self.buffer.clear()
                    self.dropping = True
                    self.on_invalid(now, 'oversize_line')
                else:
                    self.buffer.append(byte)

    def finish(self, now):
        if self.buffer:
            self.on_invalid(now, 'partial_line')
        self.buffer.clear()
        self.dropping = False


def capture(args, logs, opener, clock=time.monotonic, sleep=time.sleep):
    monitor = Monitor(logs.event, args.expected_source, args.stall, args.grace)
    def on_line(now, raw):
        try:
            sample = parse_sample(raw)
        except ValueError as exc:
            monitor.invalid(now, str(exc))
        else:
            monitor.sample(now, sample)
    framer = Framer(args.max_line, on_line, monitor.invalid)
    start = clock()
    serial_port = None
    retries = 0
    retry_at = start
    stop = 'duration'
    logs.event(start, 'capture_started', port=args.port, expected_source=args.expected_source,
               retention_bytes=args.max_bytes, retention_files=args.files,
               fsync_interval=args.fsync_interval,
               warning='Opening may change DTR/RTS; receive-only is not a hardware no-reset guarantee')
    try:
        while clock() - start < args.duration:
            now = clock()
            monitor.poll(now)
            logs.sync_due(now)
            if serial_port is None and now >= retry_at:
                try:
                    serial_port = opener(args.port, args.baud, timeout=0)
                except (OSError, IOError) as exc:
                    monitor.disconnect(now, str(exc))
                    retries += 1
                    if retries > args.reconnects:
                        stop = 'reconnect_exhausted'
                        break
                    retry_at = now + args.reconnect_delay
                else:
                    monitor.connect(now)
            if serial_port is not None:
                try:
                    data = serial_port.read(READ_LIMIT)
                except (OSError, IOError) as exc:
                    framer.finish(now)
                    monitor.disconnect(now, str(exc))
                    serial_port.close()
                    serial_port = None
                    retries += 1
                    if retries > args.reconnects:
                        stop = 'reconnect_exhausted'
                        break
                    retry_at = now + args.reconnect_delay
                else:
                    if data:
                        logs.raw(data)
                        framer.feed(data, now)
            # One bounded read per tick, even with an endless source; no drain loop.
            sleep(min(POLL, max(0, args.duration - (clock() - start))))
    except KeyboardInterrupt:
        stop = 'interrupt'
    finally:
        if serial_port is not None:
            serial_port.close()
    now = clock()
    framer.finish(now)
    monitor.poll(now)
    code = 1 if monitor.failed or monitor.ready_samples == 0 else 0
    logs.event(now, 'capture_finished', stop=stop, exit_code=code,
               valid_samples=monitor.samples, identity_valid=monitor.last is not None)
    return code


def arguments(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--port', required=True, help='Explicit path or stable device symlink')
    parser.add_argument('--expected-source', required=True)
    parser.add_argument('--output', required=True, help='Dedicated log directory')
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--duration', type=float, default=60)
    parser.add_argument('--stall', type=float, default=3)
    parser.add_argument('--grace', type=float, default=5)
    parser.add_argument('--reconnects', type=int, default=3, help='Total retry budget for the capture')
    parser.add_argument('--reconnect-delay', type=float, default=1)
    parser.add_argument('--max-bytes', type=int, default=1048576, help='Bytes per file per stream')
    parser.add_argument('--files', type=int, default=4, help='Files per stream including active file')
    parser.add_argument('--max-line', type=int, default=1024)
    parser.add_argument('--fsync-interval', type=float, default=1)
    args = parser.parse_args(argv)
    if any(not math.isfinite(getattr(args, key)) or getattr(args, key) <= 0
           for key in ('duration', 'stall', 'reconnect_delay', 'fsync_interval')):
        parser.error('duration, stall, reconnect-delay and fsync-interval must be finite and positive')
    if not math.isfinite(args.grace) or args.grace < 0:
        parser.error('grace must be finite and nonnegative')
    if args.reconnects < 0 or args.files < 1 or args.max_bytes < 4096 or not 64 <= args.max_line <= 4096 or args.baud <= 0:
        parser.error('invalid bounds: reconnects>=0, files>=1, max-bytes>=4096, 64<=max-line<=4096, baud>0')
    if not args.expected_source or len(args.expected_source) > 128 or any(c.isspace() for c in args.expected_source):
        parser.error('expected-source must be a nonempty token of at most 128 characters')
    return args


def main(argv=None):
    args = arguments(argv)
    logs = None
    code = 2
    try:
        import serial
        logs = Logs(args.output, args.max_bytes, args.files, args.fsync_interval)
        code = capture(args, logs, serial.Serial)
    except (OSError, IOError, ImportError) as exc:
        print('CAPTURE FAILED (I/O/setup): ' + str(exc), file=sys.stderr)
        code = 2
    finally:
        if logs is not None:
            try:
                logs.close()
            except OSError as exc:
                print('CAPTURE FAILED (log durability): ' + str(exc), file=sys.stderr)
                code = 2
    if code == 0:
        print('Capture complete: valid samples, no observed faults. Not proof of hardware health.')
    else:
        print('Capture failed or observed faults; inspect retained events.', file=sys.stderr)
    return code


if __name__ == '__main__':
    sys.exit(main())
