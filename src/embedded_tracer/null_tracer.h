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
};

} // namespace et
