#pragma once

#include "scope_guard.h"
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
    virtual ScopeGuard scope(const char* name) = 0;

    /// Record a counter value at the current timestamp.
    virtual void counter(const char* name, int64_t value) = 0;
};

} // namespace et
