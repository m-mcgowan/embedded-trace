#pragma once

#if !defined(EMBEDDED_TRACE_ENABLED) || EMBEDDED_TRACE_ENABLED == 0
#warning "embedded-trace: BufferTracer included but EMBEDDED_TRACE_ENABLED is not set — TRACE_SCOPE/TRACE_COUNTER will compile to no-ops. Add -DEMBEDDED_TRACE_ENABLED=1 to your build_flags."
#endif

#include "i_tracer.h"
#include "types.h"
#include <cstddef>
#include <cstdint>

namespace et {

/// Event types stored in the binary ring buffer.
enum class EventType : uint8_t {
    scope_enter = 0,
    scope_exit  = 1,
    counter     = 2,
    flow_start  = 3,
    flow_step   = 4,
    flow_end    = 5,
};

/// Visitor callback for drain(). Called once per event.
struct DrainEvent {
    TimestampUs timestamp;
    const char* name;       ///< Scope/counter/flow name (resolved from scope_id)
    EventType type;
    int64_t value;          ///< Counter value (only for EventType::counter)
    FlowId flow_id;         ///< Flow ID (only for flow_* types)
};

using DrainCallback = void (*)(void* context, const DrainEvent& event);

/**
 * @brief Binary ring buffer tracer.
 *
 * Stores trace events in a compact binary format in a caller-provided
 * buffer. Events are 7 bytes for scope enter/exit, 9 bytes for flow
 * events, and 15 bytes for counters. Designed for post-hoc capture
 * where serial output would perturb timing.
 *
 * Scope names must be string literals (pointer stability required).
 * The scope name → ID mapping uses pointer equality.
 *
 * Call drain() to iterate stored events.
 */
class BufferTracer final : public ITracer {
public:
    static constexpr size_t MAX_SCOPE_NAMES = 64;

    /// Construct with a caller-provided buffer.
    /// The buffer is NOT owned — caller manages its lifetime.
    BufferTracer(uint8_t* buffer, size_t size, TimestampFn timestamp_fn);

    ScopeGuard scope(const char* name) override;
    void counter(const char* name, int64_t value) override;
    void flow_start(const char* name, FlowId id) override;
    void flow_step(const char* name, FlowId id) override;
    void flow_end(const char* name, FlowId id) override;

    /// Drain the buffer, calling the visitor for each stored event.
    /// Events are delivered in insertion order. Does not clear the buffer.
    void drain(DrainCallback callback, void* context) const;

    /// Number of events currently in the buffer.
    size_t event_count() const { return event_count_; }

    /// True if events were lost due to buffer overflow.
    bool overflowed() const { return overflowed_; }

    /// Reset the buffer, discarding all events.
    void reset();

private:
    uint8_t* buffer_;
    size_t buffer_size_;
    TimestampFn timestamp_fn_;
    size_t write_pos_;
    size_t event_count_;
    bool overflowed_;

    // Scope name table: maps scope_id → name pointer
    const char* scope_names_[MAX_SCOPE_NAMES];
    ScopeId scope_name_count_;

    ScopeId intern_name(const char* name);
    bool write_event(EventType type, ScopeId scope_id,
                     const void* payload = nullptr, size_t payload_size = 0);

    static void scope_exit_callback(void* context, const char* name, ScopeId scope_id);
};

} // namespace et
