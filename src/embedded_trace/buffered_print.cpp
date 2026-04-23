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
    if (buffer_len_ > 0) {
        sink_.write(buffer_, buffer_len_);
        buffer_len_ = 0;
    }
}

} // namespace et
