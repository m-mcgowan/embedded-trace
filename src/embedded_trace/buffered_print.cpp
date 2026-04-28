#include "buffered_print.h"

#include <cstring>

namespace et {

BufferedPrint::BufferedPrint(Print& sink, uint8_t* buffer, size_t size,
                             ReadyFn ready_fn)
    : sink_(sink), buffer_(buffer), buffer_size_(size),
      buffer_len_(0), ready_fn_(ready_fn),
      flushed_(false), overflowed_(false) {}

size_t BufferedPrint::write(uint8_t b) {
    return write(&b, 1);
}

size_t BufferedPrint::write(const uint8_t* data, size_t size) {
    if (flushed_) {
        return sink_.write(data, size);
    }
    if (!ready_fn_()) {
        size_t room = buffer_size_ - buffer_len_;
        size_t to_copy = size < room ? size : room;
        if (to_copy > 0) {
            std::memcpy(buffer_ + buffer_len_, data, to_copy);
            buffer_len_ += to_copy;
        }
        if (to_copy < size) {
            overflowed_ = true;
        }
        // Report full consumption — caller shouldn't retry dropped bytes.
        return size;
    }
    flush_buffer();
    size_t wrote = sink_.write(data, size);
    flushed_ = true;
    return wrote;
}

void BufferedPrint::flush_buffer() {
    if (buffer_len_ == 0) return;
    // Sinks like Arduino's HWCDC can return less than `size` when their TX
    // ring is full or the host has briefly stalled — the unwritten bytes
    // would then be lost forever. Retry on partial progress, capped so a
    // permanently-stuck sink doesn't spin. If we still can't deliver the
    // whole buffer, surface it via overflowed_ so callers can observe loss.
    size_t written = 0;
    constexpr size_t kMaxRetries = 4;
    for (size_t i = 0; i < kMaxRetries && written < buffer_len_; ++i) {
        size_t n = sink_.write(buffer_ + written, buffer_len_ - written);
        if (n == 0) break;
        written += n;
    }
    if (written < buffer_len_) {
        overflowed_ = true;
    }
    buffer_len_ = 0;
}

} // namespace et
