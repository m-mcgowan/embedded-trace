#pragma once

/**
 * @brief Print adapter that buffers writes until a sink is ready.
 *
 * Problem solved: on ESP32-S3 with USB-CDC via the JTAG controller, the
 * USB link goes down during deep sleep. On wake, re-enumeration takes
 * 50-200ms. Any bytes the firmware writes to Serial before the host
 * finishes enumerating are dropped at the TX FIFO — eating the first
 * trace events of every warm wake.
 *
 * BufferedPrint wraps the real sink (e.g. Serial) with a bounded RAM
 * ring buffer and a caller-supplied readiness predicate. While the
 * predicate returns false, writes accumulate in the buffer. On the
 * first write that sees the predicate return true, the buffer is
 * flushed to the sink and the adapter enters passthrough mode for the
 * rest of its lifetime (no re-buffering on later disconnects).
 *
 * The buffer is caller-provided (no dynamic allocation).
 * Overflow policy: drop newest bytes once the buffer fills. The goal is
 * to preserve the earliest boot events (wake_cycle.B, boot.warm.B,
 * boot.hw_init.B) — later events can fall back to direct Serial writes
 * once the sink is ready.
 */

#ifdef ARDUINO
#include <Print.h>
#else
#include "native_print.h"
#endif

#include <cstddef>
#include <cstdint>

namespace et {

class BufferedPrint : public Print {
public:
    /// Predicate returning true when the downstream sink is ready to
    /// accept bytes (e.g. usb_serial_jtag_is_connected()). Called once
    /// per write() while still buffering.
    using ReadyFn = bool (*)();

    /**
     * @param sink     Downstream Print (e.g. Serial)
     * @param buffer   Caller-owned backing storage for the ring
     * @param size     Size of the backing storage in bytes
     * @param ready_fn Predicate signalling the sink is ready; must not return nullptr
     */
    BufferedPrint(Print& sink, uint8_t* buffer, size_t size, ReadyFn ready_fn);

    size_t write(uint8_t b) override;
    size_t write(const uint8_t* data, size_t size) override;

    /// True once the early-boot buffer has been flushed; subsequent
    /// writes pass straight through to the sink.
    bool is_flushed() const { return flushed_; }
    /// Current fill level of the buffer (0 once flushed).
    size_t buffered() const { return buffer_len_; }
    /// True if any byte was dropped due to buffer overflow before flush.
    bool overflowed() const { return overflowed_; }

private:
    Print& sink_;
    uint8_t* buffer_;
    size_t buffer_size_;
    size_t buffer_len_;
    ReadyFn ready_fn_;
    bool flushed_;
    bool overflowed_;

    void flush_buffer();
};

} // namespace et
