#pragma once

#include "types.h"

namespace et {

class ITracer;  // forward declaration — avoids circular include

/**
 * @brief RAII scope lifetime guard.
 *
 * Records scope-enter on construction (by the tracer that creates it),
 * scope-exit on destruction. Move-only — scopes are not copyable.
 * A null exit_fn produces a no-op guard (used by NullTracer).
 */
class ScopeGuard {
public:
    /// Callback invoked on scope exit. Tracer implementations set this
    /// to record the exit event without exposing internal API.
    using ExitFn = void (*)(void* context, const char* name, ScopeId scope_id);

    /// Default-constructed guard is a no-op: null exit_fn means the
    /// destructor does nothing. NullTracer returns these to stay
    /// branch-free at exit. Also the state left behind after move
    /// or end().
    ScopeGuard() : context_(nullptr), exit_fn_(nullptr), name_(nullptr), scope_id_(0) {}
    ScopeGuard(void* context, ExitFn exit_fn, const char* name, ScopeId scope_id);
    ~ScopeGuard();

    /// End the scope eagerly. Fires the exit callback once and disables the
    /// destructor. Safe to call on a default-constructed or already-ended
    /// guard (no-op). Use before a no-return call (e.g. esp_deep_sleep_start)
    /// where the destructor would otherwise never run.
    void end() noexcept;

    // Move-only
    ScopeGuard(ScopeGuard&& o) noexcept;
    ScopeGuard& operator=(ScopeGuard&&) noexcept;
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

private:
    void* context_;
    ExitFn exit_fn_;
    const char* name_;
    ScopeId scope_id_;
};

} // namespace et
