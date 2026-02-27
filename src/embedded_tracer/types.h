#pragma once

#include <cstdint>

namespace et {

/// Unique identifier for a scope name (auto-assigned by tracers).
using ScopeId = uint16_t;

/// Timestamp in microseconds from an arbitrary epoch.
using TimestampUs = uint32_t;

/// Timestamp function — returns microseconds from an arbitrary epoch.
using TimestampFn = TimestampUs (*)();

/// Process ID for Chrome Trace format grouping.
using ProcessId = uint8_t;

} // namespace et
