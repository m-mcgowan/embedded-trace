#include "scope_guard.h"

namespace et {

ScopeGuard::ScopeGuard(void* context, ExitFn exit_fn, const char* name, ScopeId scope_id)
    : context_(context), exit_fn_(exit_fn), name_(name), scope_id_(scope_id) {}

ScopeGuard::~ScopeGuard() {
    if (exit_fn_) {
        exit_fn_(context_, name_, scope_id_);
    }
}

void ScopeGuard::end() noexcept {
    if (exit_fn_) {
        exit_fn_(context_, name_, scope_id_);
        exit_fn_ = nullptr;
    }
}

ScopeGuard::ScopeGuard(ScopeGuard&& o) noexcept
    : context_(o.context_), exit_fn_(o.exit_fn_), name_(o.name_), scope_id_(o.scope_id_) {
    o.exit_fn_ = nullptr;  // prevent double scope-exit
}

ScopeGuard& ScopeGuard::operator=(ScopeGuard&& o) noexcept {
    if (this != &o) {
        if (exit_fn_) exit_fn_(context_, name_, scope_id_);  // end current scope
        context_ = o.context_;
        exit_fn_ = o.exit_fn_;
        name_ = o.name_;
        scope_id_ = o.scope_id_;
        o.exit_fn_ = nullptr;
    }
    return *this;
}

} // namespace et
