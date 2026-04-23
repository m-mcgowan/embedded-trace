#pragma once

#include "scope_guard.h"
#include "types.h"
#include <cstdint>

namespace et {

/**
 * @brief Abstract tracer interface.
 *
 * Begin a named scope (returns RAII guard), or record a counter value.
 * Implementations: NullTracer (production), SerialTracer (Chrome JSON),
 * BufferTracer (binary ring buffer), GPIOTracer (pin toggles).
 */
class ITracer {
public:
    virtual ~ITracer() = default;

    /// Begin a named scope. Returns an RAII guard that ends the scope
    /// on destruction. Scopes nest — the current scope becomes the
    /// parent of any scope opened before this one closes.
    ///
    /// @note @p name MUST be a string literal (or otherwise have lifetime
    ///       at least as long as the scope, with pointer stability).
    ///       BufferTracer interns names by pointer equality for its
    ///       scope_id table; passing `std::string::c_str()` or a stack
    ///       buffer produces undefined behaviour. SerialTracer is more
    ///       permissive — it copies the pointer's bytes immediately —
    ///       but code written against SerialTracer must still use string
    ///       literals to remain portable when a BufferTracer is added.
    virtual ScopeGuard scope(const char* name) = 0;

    /// Record a counter value at the current timestamp.
    virtual void counter(const char* name, int64_t value) = 0;

    /// Mark the start of a causal flow across thread/scope boundaries.
    /// The flow_id links related flow_start/step/end events.
    virtual void flow_start(const char* name, FlowId id) { (void)name; (void)id; }

    /// Mark an intermediate step in a causal flow (e.g. dequeue on a
    /// service thread after enqueue on the action thread).
    virtual void flow_step(const char* name, FlowId id) { (void)name; (void)id; }

    /// Mark the end of a causal flow.
    virtual void flow_end(const char* name, FlowId id) { (void)name; (void)id; }

    /// Set the human-readable process name shown in Perfetto.
    /// Emits a Chrome ph:M metadata event. Call once at trace start.
    /// Default no-op — implementations that have no notion of metadata
    /// (NullTracer, BufferTracer) ignore it.
    virtual void set_process_name(const char* name) { (void)name; }

    /// Set the human-readable name for a specific thread/task lane in
    /// Perfetto. Emits a Chrome ph:M metadata event for the given tid.
    /// Call once per task at trace start, e.g.:
    ///   tracer.set_thread_name(et::esp_idf_tid_fn(), "loopTask");
    /// Default no-op (see set_process_name).
    virtual void set_thread_name(ThreadId tid, const char* name) { (void)tid; (void)name; }
};

} // namespace et
