#pragma once

#include "scope_guard.h"
#include "types.h"
#include <cstdint>

namespace et {

/**
 * @brief Abstract tracer interface — all methods pure virtual.
 *
 * Every implementation must make a conscious choice about every event
 * type. To avoid stubbing methods you don't care about, inherit from
 * one of the narrow mixins instead of ITracer directly:
 *   - NoFlowTracer       — no-op flow_start/step/end
 *   - NoMetadataTracer   — no-op set_process_name/set_thread_name
 * Skipping a mixin means you're signing up to implement that concern.
 *
 * Implementations: NullTracer (production), SerialTracer (Chrome JSON),
 * BufferTracer (binary ring buffer), CompositeTracer (multiplexer),
 * GPIOTracer (pin toggles, planned).
 */
class ITracer {
public:
    virtual ~ITracer() = default;

    /// Begin a named scope. Returns an RAII guard that ends the scope
    /// on destruction. Scopes nest — the current scope becomes the
    /// parent of any scope opened before this one closes.
    ///
    /// Two calling forms:
    ///   scope("notecard", "verify")   — explicit category + name
    ///   scope("notecard.verify")      — single string; tracer auto-splits
    ///                                    on the first '.' (Perfetto "cat"
    ///                                    field). Use this form when
    ///                                    retrofitting existing dotted names.
    ///   scope("plain")                — no category (cat is null).
    ///
    /// @param cat_or_name When @p name is non-null, this is the category.
    ///                    When @p name is null, this is the full scope
    ///                    identifier (optionally dotted).
    /// @param name        The scope name, when a category is passed
    ///                    explicitly. Null means use single-arg form.
    ///
    /// @note Both pointers MUST be string literals (or otherwise have
    ///       lifetime at least as long as the scope, with pointer
    ///       stability). BufferTracer interns by pointer equality on
    ///       the (cat, name) pair; passing `std::string::c_str()` or
    ///       a stack buffer produces undefined behaviour. SerialTracer
    ///       is more permissive — it copies the bytes immediately —
    ///       but code written against SerialTracer must still use
    ///       string literals to remain portable when a BufferTracer
    ///       is added.
    virtual ScopeGuard scope(const char* cat_or_name, const char* name = nullptr) = 0;

    /// Record a counter value at the current timestamp.
    virtual void counter(const char* name, int64_t value) = 0;

    /// Mark the start of a causal flow across thread/scope boundaries.
    /// The flow_id links related flow_start/step/end events. Inherit
    /// from NoFlowTracer to take no-op defaults for the three flow_*
    /// methods.
    virtual void flow_start(const char* name, FlowId id) = 0;

    /// Mark an intermediate step in a causal flow (e.g. dequeue on a
    /// service thread after enqueue on the action thread).
    virtual void flow_step(const char* name, FlowId id) = 0;

    /// Mark the end of a causal flow.
    virtual void flow_end(const char* name, FlowId id) = 0;

    /// Set the human-readable process name shown in Perfetto.
    /// Emits a Chrome ph:M metadata event. Call once at trace start.
    /// Inherit from NoMetadataTracer to take no-op defaults for the
    /// two set_*_name methods.
    virtual void set_process_name(const char* name) = 0;

    /// Set the human-readable name for a specific thread/task lane in
    /// Perfetto. Emits a Chrome ph:M metadata event for the given tid.
    /// Call once per task at trace start, e.g.:
    ///   tracer.set_thread_name(et::esp_idf_tid_fn(), "loopTask");
    virtual void set_thread_name(ThreadId tid, const char* name) = 0;
};

} // namespace et
