#pragma once

#include "i_tracer.h"

namespace et {

/**
 * @brief No-op tracer. Production default.
 *
 * ScopeGuard with null exit callback — destructor is a no-op branch.
 * The compiler can often eliminate the guard entirely.
 */
class NullTracer final : public ITracer {
public:
    static NullTracer& instance() {
        static NullTracer inst;
        return inst;
    }

    ScopeGuard scope(const char* name) override {
        return ScopeGuard(nullptr, nullptr, name, 0);
    }

    void counter(const char*, int64_t) override {}

    void flow_start(const char*, FlowId) override {}
    void flow_step(const char*, FlowId) override {}
    void flow_end(const char*, FlowId) override {}

    void set_process_name(const char*) override {}
    void set_thread_name(ThreadId, const char*) override {}
};

} // namespace et
