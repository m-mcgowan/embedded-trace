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

/// Thread ID for Chrome Trace format swim lanes.
using ThreadId = uint32_t;

/// Thread ID function — returns the current thread/task ID.
/// On FreeRTOS, typically casts xTaskGetCurrentTaskHandle() to uint32_t.
using ThreadIdFn = ThreadId (*)();

/// Flow ID for cross-thread causal links.
using FlowId = uint16_t;

} // namespace et
