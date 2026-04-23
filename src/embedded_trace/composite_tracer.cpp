#include "composite_tracer.h"

namespace et {

CompositeTracer::CompositeTracer(ITracer** tracers, size_t count)
    : children_{}, child_count_(0), scope_stack_{}, stack_depth_(0) {
    if (count > MAX_CHILDREN) count = MAX_CHILDREN;
    for (size_t i = 0; i < count; ++i) {
        children_[i] = tracers[i];
    }
    child_count_ = count;
}

ScopeGuard CompositeTracer::scope(const char* cat_or_name, const char* name) {
    if (stack_depth_ >= MAX_NESTING) {
        // Nesting too deep — return no-op guard
        return ScopeGuard();
    }

    ScopeState& state = scope_stack_[stack_depth_];
    state.count = child_count_;
    for (size_t i = 0; i < child_count_; ++i) {
        state.guards[i] = children_[i]->scope(cat_or_name, name);
    }
    stack_depth_++;

    // Return a guard that pops the stack on exit. cat/name are not used
    // by the composite itself — each child holds its own guard.
    return ScopeGuard(this, scope_exit_callback, nullptr, nullptr, 0);
}

void CompositeTracer::counter(const char* name, int64_t value) {
    for (size_t i = 0; i < child_count_; ++i) {
        children_[i]->counter(name, value);
    }
}

void CompositeTracer::flow_start(const char* name, FlowId id) {
    for (size_t i = 0; i < child_count_; ++i) {
        children_[i]->flow_start(name, id);
    }
}

void CompositeTracer::flow_step(const char* name, FlowId id) {
    for (size_t i = 0; i < child_count_; ++i) {
        children_[i]->flow_step(name, id);
    }
}

void CompositeTracer::flow_end(const char* name, FlowId id) {
    for (size_t i = 0; i < child_count_; ++i) {
        children_[i]->flow_end(name, id);
    }
}

void CompositeTracer::set_process_name(const char* name) {
    for (size_t i = 0; i < child_count_; ++i) {
        children_[i]->set_process_name(name);
    }
}

void CompositeTracer::set_thread_name(ThreadId tid, const char* name) {
    for (size_t i = 0; i < child_count_; ++i) {
        children_[i]->set_thread_name(tid, name);
    }
}

void CompositeTracer::scope_exit_callback(void* context, const char* /*cat*/,
                                          const char* /*name*/, ScopeId /*scope_id*/) {
    auto* self = static_cast<CompositeTracer*>(context);
    if (self->stack_depth_ == 0) return;

    self->stack_depth_--;
    ScopeState& state = self->scope_stack_[self->stack_depth_];

    // Destroy child guards in reverse order (LIFO)
    for (size_t i = state.count; i > 0; --i) {
        state.guards[i - 1] = ScopeGuard();  // Move-assign default → triggers exit
    }
}

} // namespace et
