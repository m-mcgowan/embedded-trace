#pragma once

#if !defined(EMBEDDED_TRACE_ENABLED) || EMBEDDED_TRACE_ENABLED == 0
#warning "embedded-trace: CompositeTracer included but EMBEDDED_TRACE_ENABLED is not set — TRACE_SCOPE/TRACE_COUNTER will compile to no-ops. Add -DEMBEDDED_TRACE_ENABLED=1 to your build_flags."
#endif

#include "i_tracer.h"
#include <cstddef>

namespace et {

/**
 * @brief Multiplexer tracer — delegates to up to 4 child tracers.
 *
 * Typical use: SerialTracer for scope naming + BufferTracer for
 * high-precision capture, or any combination of tracer implementations.
 *
 * Each child's scope() returns its own ScopeGuard. CompositeTracer
 * stores these internally and releases them all when the composite
 * guard is destroyed.
 */
class CompositeTracer final : public ITracer {
public:
    static constexpr size_t MAX_CHILDREN = 4;

    /// Construct with an array of child tracers.
    CompositeTracer(ITracer** tracers, size_t count);

    ScopeGuard scope(const char* cat_or_name, const char* name = nullptr) override;
    void counter(const char* name, int64_t value) override;
    void flow_start(const char* name, FlowId id) override;
    void flow_step(const char* name, FlowId id) override;
    void flow_end(const char* name, FlowId id) override;
    void set_process_name(const char* name) override;
    void set_thread_name(ThreadId tid, const char* name) override;

private:
    ITracer* children_[MAX_CHILDREN];
    size_t child_count_;

    // Storage for child scope guards (one set per active composite scope).
    // For simplicity, supports one active scope at a time via the callback.
    // Nested scopes work because each ScopeGuard captures its own exit data.
    struct ScopeState {
        ScopeGuard guards[MAX_CHILDREN];
        size_t count;
    };

    // We use a small stack of scope states for nesting
    static constexpr size_t MAX_NESTING = 8;
    ScopeState scope_stack_[MAX_NESTING];
    size_t stack_depth_;

    static void scope_exit_callback(void* context, const char* cat,
                                    const char* name, ScopeId scope_id);
};

} // namespace et
