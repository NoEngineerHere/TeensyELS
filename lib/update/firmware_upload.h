#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace FirmwareUpload {
inline constexpr size_t imageHeaderSize = 24, segmentHeaderSize = 8, appDescSize = 256;
inline constexpr size_t chipOffset = 12, segmentSizeOffset = imageHeaderSize + 4;
inline constexpr size_t appDescOffset = imageHeaderSize + segmentHeaderSize;
inline constexpr size_t descriptorOffset = appDescOffset + appDescSize;
inline constexpr uint32_t appDescMagic = 0xabcd5432;
inline constexpr uint8_t imageMagic = 0xe9, maxSegments = 16;

enum class Result { Installed, TransferFailed, WrongImage, WrongHardware, EraseFailed,
                    WriteFailed, VerifyFailed, SelectFailed, Cancelled };

inline size_t contentLength(const char* value, size_t expected, size_t max) {
  if (!*value || expected == 0 || expected > max) return 0;
  size_t parsed = 0;
  for (; *value; ++value) {
    if (*value < '0' || *value > '9') return 0;
    const unsigned digit = *value - '0';
    if (parsed > max / 10 || (parsed == max / 10 && digit > max % 10)) return 0;
    parsed = parsed * 10 + digit;
  }
  return parsed == expected ? parsed : 0;
}

inline uint32_t readLe32(const uint8_t* p) {
  return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

// Stream::read returns a positive byte count, or <=0 on interruption. The writer
// owns any live flash handle and aborts it unless finish() has consumed it.
template<class Stream, class Writer, size_t DescriptorSize>
Result install(Stream& stream, Writer& writer, size_t length,
               const std::array<uint8_t, DescriptorSize>& descriptor) {
  constexpr size_t prefixSize = descriptorOffset + DescriptorSize;
  uint8_t buffer[1024];
  static_assert(prefixSize <= sizeof(buffer));
  if (length < prefixSize) return Result::WrongImage;
  size_t received = 0;
  while (received < prefixSize) {
    const int n = stream.read(buffer + received, prefixSize - received);
    if (n <= 0) return Result::TransferFailed;
    received += n;
  }
  const auto segmentSize = readLe32(buffer + segmentSizeOffset);
  if (buffer[0] != imageMagic || buffer[1] == 0 || buffer[1] > maxSegments ||
      buffer[chipOffset] != 0 || buffer[chipOffset+1] != 0 ||
      segmentSize < appDescSize + DescriptorSize || segmentSize > length - appDescOffset ||
      readLe32(buffer + appDescOffset) != appDescMagic)
    return Result::WrongImage;
  if (std::memcmp(buffer + descriptorOffset, descriptor.data(), DescriptorSize) != 0)
    return Result::WrongHardware;
  if (!stream.active()) return Result::Cancelled;
  if (!writer.begin(length)) return Result::EraseFailed;
  if (!writer.write(buffer, received)) return Result::WriteFailed;
  stream.progress(received);
  while (received < length) {
    const int n = stream.read(buffer, std::min(length - received, sizeof(buffer)));
    if (n <= 0) return Result::TransferFailed;
    if (!writer.write(buffer, n)) return Result::WriteFailed;
    received += n;
    stream.progress(received);
  }
  if (!stream.active()) return Result::Cancelled;
  if (!writer.finish()) return Result::VerifyFailed;
  if (!stream.active()) return Result::Cancelled;
  return writer.select() ? Result::Installed : Result::SelectFailed;
}
}
