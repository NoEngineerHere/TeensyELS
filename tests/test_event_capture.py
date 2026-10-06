"""Simulated unit boundaries; optional real pyserial Linux PTY, never hardware."""
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location('capture', Path(__file__).resolve().parents[1] / 'scripts/els_event_capture.py')
assert SPEC is not None and SPEC.loader is not None
capture = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(capture)


def line(us=100, loop=1, timer=1, frames=1, source='abc', ready=1, fault=0):
    return (f'ELS_PROGRESS source={source} us={us} loop={loop} timer={timer} '
            f'frames={frames} oled_fault={fault} ready={ready}').encode()


def args(**updates):
    result = capture.arguments(['--port', '/simulated', '--expected-source', 'abc', '--output', '/unused'])
    result.duration = 0.4
    result.stall = 0.15
    result.grace = 0.05
    result.reconnect_delay = 0.05
    for key, value in updates.items():
        setattr(result, key, value)
    return result


class FakeClock:
    def __init__(self):
        self.now = 0.0
    def __call__(self):
        return self.now
    def sleep(self, interval):
        self.now += interval


class FakeSerial:
    def __init__(self, chunks, forever=None):
        self.chunks = iter(chunks)
        self.forever = forever
        self.reads = []
        self.closed = False
    def read(self, size):
        self.reads.append(size)
        item = next(self.chunks, self.forever or b'')
        if isinstance(item, Exception):
            raise item
        return item[:size]
    def write(self, data):
        raise AssertionError('receive-only recorder attempted serial write')
    def close(self):
        self.closed = True


class MemoryLogs:
    def __init__(self):
        self.events = []
        self.raw_bytes = bytearray()
    def event(self, now, kind, **details):
        self.events.append(dict(event=kind, monotonic=now, **details))
    def raw(self, data):
        self.raw_bytes.extend(data)
    def sync_due(self, now):
        pass
    def close(self):
        pass


class ParserTests(unittest.TestCase):
    def test_default_baud_matches_firmware_uart(self):
        self.assertEqual(args().baud, 921600)

    def test_legacy_active_outputs_are_reported_as_fault(self):
        raw = (f'{capture.LEGACY} source={capture.LEGACY_SOURCE} ms=2 fg=3 timer_done=4 '
               'frames=5 crash=0 gpio_outputs=3 motion=INHIBITED mode=0').encode()
        self.assertTrue(capture.parse_sample(raw)['fault'])

    def test_exact_schemas(self):
        sample = capture.parse_sample(line() + b' optional=yes')
        self.assertEqual(sample['uptime'], 100)
        self.assertEqual(sample['scale'], 1e-6)
        raw = (f'{capture.LEGACY} source={capture.LEGACY_SOURCE} ms=2 fg=3 timer_done=4 '
               'frames=5 crash=0 gpio_outputs=0x03 motion=INHIBITED mode=0 extra=ok').encode()
        sample = capture.parse_sample(raw)
        self.assertEqual((sample['scale'], sample['loop'], sample['timer']), (.001, 3, 4))
        self.assertTrue(sample['ready'])
        for old, new in ((b'motion=INHIBITED', b'motion=ON'), (b'mode=0', b'mode=1'),
                         (capture.LEGACY_SOURCE.encode(), b'other'), (b'gpio_outputs=0x03', b'gpio_outputs=-1')):
            with self.subTest(old=old), self.assertRaises(ValueError):
                capture.parse_sample(raw.replace(old, new))

    def test_malformed(self):
        cases = [line(us=4294967296), line(us=-1), line(ready=2), b'boot only',
                 line() + b' loop=1', line().replace(b' loop=1', b''),
                 line().replace(b' loop=1', b' loop=+1'), b'\xff', line() + b' bad']
        for raw in cases:
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                capture.parse_sample(raw)

    def test_argument_bounds(self):
        for option, value in [('--stall', 'nan'), ('--grace', '-1'), ('--duration', 'inf'),
                              ('--files', '0'), ('--max-bytes', '100'), ('--max-line', '999999')]:
            with self.subTest(option=option), contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
                capture.arguments(['--port', '/simulated', '--expected-source', 'abc', '--output', '/unused', option, value])


class LogTests(unittest.TestCase):
    def test_flush_before_exit_timestamps_and_fsync(self):
        with tempfile.TemporaryDirectory() as directory:
            sink = capture.Logs(directory, 4096, 2, 1)
            sink.raw(b'bytes')
            sink.event(1, 'test')
            self.assertEqual((Path(directory) / 'raw.bin').read_bytes(), b'bytes')
            event = json.loads((Path(directory) / 'events.jsonl').read_text())
            self.assertEqual(event['monotonic'], 1)
            self.assertTrue(event['utc'].endswith('+00:00'))
            with patch.object(capture.os, 'fsync') as sync:
                sink.sync_due(sink.last_sync + 1)
                self.assertEqual(sync.call_count, 2)
            sink.close()

    def test_bounded_rotation_and_json_records(self):
        with tempfile.TemporaryDirectory() as directory:
            sink = capture.Logs(directory, 4096, 2, 1)
            sink.raw(b'x' * 20000)
            for index in range(100):
                sink.event(index, 'test', text='x' * 100)
            sink.close()
            files = list(Path(directory).iterdir())
            self.assertLessEqual(len(files), 4)
            self.assertTrue(all(path.stat().st_size <= 4096 for path in files))
            for path in files:
                if path.name.startswith('events'):
                    for record in path.read_text().splitlines():
                        self.assertEqual(json.loads(record)['event'], 'test')
            # Reducing retention removes only numeric managed archives.
            (Path(directory) / 'raw.bin.9').write_bytes(b'old')
            (Path(directory) / 'raw.bin.notes').write_bytes(b'keep')
            sink = capture.Logs(directory, 4096, 1, 1)
            sink.close()
            self.assertFalse((Path(directory) / 'raw.bin.9').exists())
            self.assertFalse((Path(directory) / 'raw.bin.1').exists())
            self.assertTrue((Path(directory) / 'raw.bin.notes').exists())
            (Path(directory) / 'raw.bin').write_bytes(b'x' * 5000)
            sink = capture.Logs(directory, 4096, 2, 1)
            sink.close()
            self.assertTrue(all(path.stat().st_size <= 4096 for path in Path(directory).iterdir()))

    def test_disk_failures_propagate(self):
        with tempfile.TemporaryDirectory() as directory:
            sink = capture.Logs(directory, 4096, 2, 1)
            with patch.object(sink.binary, 'write', side_effect=OSError('disk full')):
                with self.assertRaisesRegex(OSError, 'disk full'):
                    sink.raw(b'data')
            with patch.object(capture.os, 'fsync', side_effect=OSError('fsync failed')):
                with self.assertRaisesRegex(OSError, 'fsync failed'):
                    sink.close()

    def test_main_no_success_on_disk_error_or_close_error(self):
        try:
            import serial  # noqa: F401
        except ImportError:
            self.skipTest('pyserial unavailable')
        for close_error in (False, True):
            output = io.StringIO()
            sink = MemoryLogs()
            sink.close = lambda: None
            if close_error:
                sink.close = lambda: (_ for _ in ()).throw(OSError('close error'))
            with patch.object(capture, 'Logs', return_value=sink), patch.object(
                    capture, 'capture', return_value=0, side_effect=None if close_error else OSError('disk full')):
                with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
                    code = capture.main(['--port', '/simulated', '--expected-source', 'abc', '--output', '/unused'])
            self.assertEqual(code, 2)
            self.assertNotIn('Capture complete', output.getvalue())
            self.assertIn('CAPTURE FAILED', output.getvalue())


class MonitorTests(unittest.TestCase):
    def setUp(self):
        self.logs = MemoryLogs()
        self.monitor = capture.Monitor(self.logs.event, 'abc', 3, 5)
        self.monitor.connect(0)
    def kinds(self):
        return [event['event'] for event in self.logs.events]
    def sample(self, now, **values):
        self.monitor.sample(now, capture.parse_sample(line(**values)))

    def test_grace_stalls_restores_and_display_ambiguity(self):
        self.sample(0)
        self.monitor.poll(2)
        self.assertNotIn('frames_stalled', self.kinds())
        self.sample(4, us=4000100, loop=2, timer=2)
        self.monitor.poll(5)
        self.assertIn('frames_stalled', self.kinds())
        self.assertNotIn('loop_stalled', self.kinds())
        event = next(event for event in self.logs.events if event['event'] == 'frames_stalled')
        self.assertIn('not screen failure', event['ambiguity'])
        self.monitor.poll(5.1)
        self.assertEqual(self.kinds().count('frames_stalled'), 1)
        self.sample(6, us=6000100, loop=3, timer=3, frames=2)
        self.assertIn('frames_stalled_restored', self.kinds())
        self.assertTrue(self.monitor.failed)  # Restoration does not erase prior faults.

    def test_heartbeat_missing_and_identity_loss(self):
        self.sample(0)
        self.monitor.poll(5)
        self.assertIn('heartbeat_missing', self.kinds())
        self.monitor.disconnect(6, 'removed')
        self.assertIsNone(self.monitor.last)
        self.monitor.connect(7)
        self.assertIsNone(self.monitor.last)
        self.sample(8, source='wrong')
        self.assertIsNone(self.monitor.last)
        self.sample(9)
        self.assertIsNotNone(self.monitor.last)
        self.assertIn('source_mismatch_restored', self.kinds())
        self.assertEqual(self.kinds().count('identity_verified'), 2)

    def test_fault_and_invalid_invalidate(self):
        self.sample(0, fault=1)
        self.sample(1)
        self.assertIn('firmware_fault_restored', self.kinds())
        self.monitor.invalid(2, 'oversize_line')
        self.assertIsNone(self.monitor.last)
        self.assertIn('identity_lost', self.kinds())

    def test_uptime_and_counter_wrap(self):
        self.sample(0, us=4294967200, loop=4294967295, timer=4294967295, frames=4294967295)
        self.sample(1, us=999904, loop=0, timer=0, frames=0)
        self.assertIn('uptime_wrap_candidate', self.kinds())
        self.assertEqual(self.kinds().count('counter_wrap'), 3)
        self.assertFalse(self.monitor.failed)
        self.assertNotIn('reset_candidate', self.kinds())

    def test_ms_uptime_wrap(self):
        monitor = capture.Monitor(self.logs.event, capture.LEGACY_SOURCE, 3, 5)
        monitor.connect(0)
        for now, uptime in ((0, 4294967200), (1, 904)):
            raw = (f'{capture.LEGACY} source={capture.LEGACY_SOURCE} ms={uptime} fg={now+1} '
                   f'timer_done={now+1} frames={now+1} crash=0 gpio_outputs=0 motion=INHIBITED mode=0').encode()
            monitor.sample(now, capture.parse_sample(raw))
        self.assertIn('uptime_wrap_candidate', self.kinds())
        self.assertFalse(monitor.failed)

    def test_reset_candidate_not_power_cause(self):
        self.sample(0, us=10000000, loop=500, timer=500, frames=500)
        self.sample(1, us=100, loop=1, timer=1, frames=1)
        self.assertIn('reset_candidate', self.kinds())
        self.assertIn('counter_reset_candidate', self.kinds())
        self.assertTrue(self.monitor.failed)
        self.assertEqual(self.monitor.ready_at, 1)
        self.monitor.poll(2)
        self.assertNotIn('frames_stalled', self.kinds())

    def test_not_ready_not_healthy(self):
        self.sample(0, ready=0)
        self.sample(5, ready=0)
        self.monitor.poll(5)
        self.assertIn('initialization_not_ready', self.kinds())
        self.assertEqual(self.monitor.ready_samples, 0)

    def test_host_gap_disclosed(self):
        self.monitor.poll(0)
        self.monitor.poll(10)
        self.assertIn('host_poll_gap', self.kinds())
        self.monitor.poll(10.1)
        self.assertIn('host_poll_gap_restored', self.kinds())


class CaptureTests(unittest.TestCase):
    def test_real_boot_report_then_progress_is_not_malformed_telemetry(self):
        boot = (b'\nCurrent capturing firmware: abc\nBuild: test\nTarget: teensy41\n'
                b'Reset flags/reason: 0x00000001\nNo Crash Data To Report\n')
        port = FakeSerial([boot, line(us=100000, loop=1, timer=1, frames=1) + b'\n',
                           line(us=200000, loop=2, timer=2, frames=2) + b'\n'])
        code, logs, _ = self.run_capture([port], stall=1, grace=.2)
        self.assertEqual(code, 0)
        self.assertIn('boot_report_text', [event['event'] for event in logs.events])
        self.assertEqual(sum(event['event'] == 'sample' for event in logs.events), 2)

    def test_wrong_boot_source_is_not_blessed_by_later_progress(self):
        port = FakeSerial([b'Current capturing firmware: wrong\n', line() + b'\n'])
        code, logs, _ = self.run_capture([port], stall=1)
        self.assertEqual(code, 1)
        self.assertIn('source_mismatch', [event['event'] for event in logs.events])

    def test_capture_duration_truncation_is_not_a_device_fault(self):
        port = FakeSerial([line() + b'\n', b'ELS_PROGRESS source=abc us='])
        code, logs, _ = self.run_capture([port], stall=1)
        self.assertEqual(code, 0)
        self.assertIn('capture_truncated', [event['event'] for event in logs.events])

    def test_boot_text_alone_never_counts_as_progress(self):
        port = FakeSerial([b'Current capturing firmware: abc\nBuild: test\n'])
        code, logs, _ = self.run_capture([port], stall=1)
        self.assertEqual(code, 1)
        self.assertFalse(any(event['event'] == 'sample' for event in logs.events))

    def test_unknown_els_schema_still_invalidates_proof(self):
        port = FakeSerial([line() + b'\n', b'ELS_UNKNOWN source=abc\n'])
        code, logs, _ = self.run_capture([port], stall=1)
        self.assertEqual(code, 1)
        self.assertIn('identity_lost', [event['event'] for event in logs.events])

    def run_capture(self, ports, **options):
        clock, logs = FakeClock(), MemoryLogs()
        ports = iter(ports)
        calls = []
        def opener(port, baud, timeout):
            calls.append((port, baud, timeout))
            item = next(ports)
            if isinstance(item, Exception):
                raise item
            return item
        code = capture.capture(args(**options), logs, opener, clock, clock.sleep)
        return code, logs, calls

    def test_receive_only_bounded_reads_endless_stream(self):
        port = FakeSerial([], forever=b'x' * capture.READ_LIMIT)
        code, logs, calls = self.run_capture([port])
        self.assertEqual(code, 1)
        self.assertLessEqual(len(port.reads), 9)
        self.assertTrue(all(size == capture.READ_LIMIT for size in port.reads))
        self.assertEqual(calls[0][2], 0)
        self.assertTrue(port.closed)
        self.assertIn('heartbeat_missing', [event['event'] for event in logs.events])

    def test_missing_device_retries_bounded(self):
        code, logs, calls = self.run_capture([OSError('missing')] * 4, reconnects=2)
        self.assertEqual(code, 1)
        self.assertEqual(len(calls), 3)
        self.assertEqual(logs.events[-1]['stop'], 'reconnect_exhausted')

    def test_reconnect_requires_fresh_source(self):
        first = FakeSerial([line() + b'\n', OSError('lost')])
        second = FakeSerial([line(source='other') + b'\n'])
        code, logs, calls = self.run_capture([first, second])
        self.assertEqual(code, 1)
        self.assertEqual(len(calls), 2)
        self.assertFalse(logs.events[-1]['identity_valid'])
        self.assertIn('source_mismatch', [event['event'] for event in logs.events])

    def test_partial_unknown_boot_only_and_interrupt(self):
        for raw in (b'boot\n', line(), line(ready=0) + b'\n'):
            port = FakeSerial([raw])
            code, logs, _ = self.run_capture([port])
            self.assertEqual(code, 1)
        clock, logs = FakeClock(), MemoryLogs()
        port = FakeSerial([line() + b'\n'])
        def interrupt(seconds):
            raise KeyboardInterrupt
        code = capture.capture(args(), logs, lambda *a, **kw: port, clock, interrupt)
        self.assertEqual(code, 0)
        self.assertTrue(port.closed)
        self.assertEqual(logs.events[-1]['stop'], 'interrupt')

    def test_oversize_then_valid_and_bounded_buffer(self):
        good, invalid = [], []
        framer = capture.Framer(64, lambda now, raw: good.append(raw),
                                lambda now, reason: invalid.append(reason))
        for _ in range(100):
            framer.feed(b'x' * 4096, 0)
            self.assertLessEqual(len(framer.buffer), 64)
        framer.feed(b'\nabc\npartial', 1)
        framer.finish(2)
        self.assertEqual(good, [b'abc'])
        self.assertEqual(invalid, ['oversize_line', 'partial_line'])

    def test_healthy_progress_and_live_persistence(self):
        with tempfile.TemporaryDirectory() as directory:
            logs = capture.Logs(directory, 4096, 2, 1)
            clock = FakeClock()
            port = FakeSerial([line(us=i * 50000, loop=i, timer=i, frames=i) + b'\n' for i in range(10)])
            def sleep(interval):
                self.assertGreater((Path(directory) / 'raw.bin').stat().st_size, 0)
                self.assertIn('sample', (Path(directory) / 'events.jsonl').read_text())
                clock.sleep(interval)
            code = capture.capture(args(), logs, lambda *a, **kw: port, clock, sleep)
            logs.close()
            self.assertEqual(code, 0)


class PtySmoke(unittest.TestCase):
    @unittest.skipUnless(sys.platform.startswith('linux'), 'Linux PTY test')
    def test_real_pyserial_cli(self):
        try:
            import serial  # noqa: F401
        except ImportError:
            self.skipTest('pyserial unavailable; use requirements-capture.txt')
        import pty
        master, slave = pty.openpty()
        stop = threading.Event()
        def producer():
            index = 0
            while not stop.wait(.05):
                index += 1
                os.write(master, line(us=index * 50000, loop=index, timer=index, frames=index) + b'\n')
        writer = threading.Thread(target=producer)
        try:
            with tempfile.TemporaryDirectory() as directory:
                writer.start()
                result = subprocess.run([sys.executable, str(SPEC.origin), '--port', os.ttyname(slave),
                                         '--expected-source', 'abc', '--output', directory, '--duration', '.6',
                                         '--grace', '.2', '--stall', '.2'], capture_output=True, text=True, timeout=5)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                records = [json.loads(raw) for raw in (Path(directory) / 'events.jsonl').read_text().splitlines()]
                self.assertGreater(sum(record['event'] == 'sample' for record in records), 3)
                self.assertEqual(records[-1]['exit_code'], 0)
                self.assertIn('ELS_PROGRESS', (Path(directory) / 'raw.bin').read_text())
        finally:
            stop.set()
            if writer.ident is not None:
                writer.join()
            os.close(master)
            os.close(slave)


if __name__ == '__main__':
    unittest.main()
