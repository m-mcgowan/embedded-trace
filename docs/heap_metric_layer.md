# Heap Metric Layer — ESP32 Extension

An ESP32-specific metric layer for embedded-trace that provides per-scope heap
attribution. Uses ESP-IDF's `heap_trace_*` standalone APIs to track allocations
within named scopes, integrating with the existing scope tree.

## Motivation

The firmware already has several ad-hoc heap tracking patterns:

1. **Per-test global delta** (`pio-test-runner`): `PtrTestListener` snapshots
   `esp_get_free_heap_size()` before/after each doctest test case. Python-side
   `MemoryTracker` (embedded-bridge) aggregates these into a "Memory Report".
   Effective for detecting gross leaks but provides no attribution — you know a
   test leaked 2KB, but not which operation within the test.

2. **Inline trace start/stop** (`test_binary_capture_manager.cpp`):
   ```cpp
   #if CONFIG_HEAP_TRACING_STANDALONE
   heap_trace_start(HEAP_TRACE_LEAKS);
   #endif
   // ... test body ...
   #if CONFIG_HEAP_TRACING_STANDALONE
   heap_trace_stop();
   heap_trace_dump();
   #endif
   ```
   This wraps a region of code with allocation tracing but is manual, not
   composable, and not integrated with the scope tree.

3. **Scattered `esp_get_free_heap_size()` printf** calls in various test files.
   Zero structure, zero integration, zero reuse.

A `HeapTracer` that integrates with `ITracer` would replace all three patterns
with a single consistent mechanism.

## Design

### `HeapTracer` class — `embedded_trace_esp32/heap_tracer.h`

```cpp
namespace et {
namespace esp32 {

struct HeapTracerConfig {
    /// Maximum scopes to track simultaneously (nesting depth)
    size_t max_depth = 8;

    /// Bytes of heap change that triggers a warning (per scope exit)
    int64_t leak_threshold = 512;

    /// Whether to call heap_trace_dump() when a leak exceeds threshold
    bool auto_dump = true;

    /// Record buffer for ESP-IDF heap_trace_init_standalone()
    heap_trace_record_t* records = nullptr;
    size_t num_records = 0;
};

class HeapTracer final : public ITracer {
public:
    HeapTracer(HeapTracerConfig config, ITracer* delegate = nullptr);

    ScopeGuard scope(const char* name) override;
    void counter(const char* name, int64_t value) override;

    /// Total detected leak across all scopes since creation
    int64_t total_leaked() const;

    /// Per-scope leak info for the most recent scope of each name
    struct ScopeHeapInfo {
        const char* name;
        size_t heap_before;
        size_t heap_after;
        int64_t delta;  // positive = leak
    };

private:
    HeapTracerConfig config_;
    ITracer* delegate_;     // Optional inner tracer (e.g. SerialTracer)
    // Stack of heap snapshots for nested scopes
    // ...
};

}}
```

### Scope lifecycle

On `scope()` entry:
1. Snapshot `esp_get_free_heap_size()`
2. If at depth 0 (outermost scope) and `config_.records` is set:
   `heap_trace_start(HEAP_TRACE_LEAKS)`
3. Push snapshot onto internal stack
4. If `delegate_` is set, call `delegate_->scope(name)` for tracing output

On scope exit (via `ScopeGuard` callback):
1. Pop snapshot from stack
2. Compute delta = `before - esp_get_free_heap_size()`
3. Emit `counter(name + "_heap_delta", delta)` to delegate (if set)
4. If delta > `leak_threshold` and `auto_dump`: `heap_trace_stop(); heap_trace_dump()`
5. If at depth 0: `heap_trace_stop()`

### Composition with CompositeTracer

```cpp
// Production: zero cost
NullTracer production_tracer;

// Bench profiling: serial trace + heap attribution
SerialTracer serial(Serial, esp_timer_get_time);
HeapTracer heap({.leak_threshold = 256, .auto_dump = true}, &serial);
// Or use CompositeTracer for multiple independent concerns:
CompositeTracer combined({&serial, &heap});
```

### Test helper: `ScopedHeapTrace`

For test code that doesn't use the full tracer infrastructure, a lightweight
RAII wrapper:

```cpp
namespace et {
namespace esp32 {

struct ScopedHeapTrace {
    size_t heap_before;

    ScopedHeapTrace() : heap_before(esp_get_free_heap_size()) {
#if CONFIG_HEAP_TRACING_STANDALONE
        heap_trace_start(HEAP_TRACE_LEAKS);
#endif
    }

    /// Returns bytes leaked (positive = leak). Dumps if above threshold.
    int64_t stop_and_check(int64_t threshold = 512, Print* log = nullptr) {
        size_t heap_after = esp_get_free_heap_size();
        int64_t delta = (int64_t)heap_before - (int64_t)heap_after;
#if CONFIG_HEAP_TRACING_STANDALONE
        heap_trace_stop();
        if (delta > threshold) {
            if (log) {
                log->printf("HEAP LEAK: %lld bytes (before=%u, after=%u)\n",
                            delta, (unsigned)heap_before, (unsigned)heap_after);
            }
            heap_trace_dump();
        }
#endif
        return delta;
    }
};

}}
```

## Dependencies

- **ESP-IDF**: `esp_heap_trace.h` (part of `heap` component)
- **Kconfig**: `CONFIG_HEAP_TRACING_STANDALONE=y` must be enabled
  - Must be enabled in the project's sdkconfig fragments
  - `CONFIG_HEAP_TRACING_STACK_DEPTH=2` (2 stack frames per allocation record)
- **Record buffer**: Caller provides `heap_trace_record_t[]` — typically a static
  array in the test file (200 records ~= 3.2KB at 16 bytes/record)

## Integration with existing tooling

| Layer | Current | With HeapTracer |
|-------|---------|-----------------|
| **Device** | Ad-hoc `esp_get_free_heap_size()` | `HeapTracer` emits structured per-scope counters |
| **Serial protocol** | `PTR:MEM:BEFORE/AFTER` per test | Same, plus Chrome JSON counters for per-scope deltas |
| **Host Python** | `MemoryTracker` in embedded-bridge | Extend `EventCapture` to parse heap counter events |
| **Visualization** | None (manual printf reading) | Perfetto counter tracks show heap timeline per scope |

## Implementation plan

1. Add `heap_tracer.h` and `heap_tracer.cpp` to `src/embedded_trace_esp32/`
2. Add `ScopedHeapTrace` to same directory (header-only)
3. Native tests: mock `esp_get_free_heap_size()` and `heap_trace_*` for unit testing
4. Integration test: use in `test_binary_capture_manager.cpp` to replace existing
   inline `#if CONFIG_HEAP_TRACING_STANDALONE` blocks
5. Add Perfetto counter visualization example to docs

## Relationship to other metric layers

The heap metric layer is one of several planned metric layers that compose onto
the scope tree:

| Metric Layer | Source | Status |
|-------------|--------|--------|
| **Time** | `esp_timer_get_time()` | Built-in (SerialTracer timestamps) |
| **Power** | PPK2 via `ppk2-python` | External correlation via scope timestamps |
| **Heap** | `esp_get_free_heap_size()` + `heap_trace_*` | This document |
| **GPIO** | `GPIOTracer` (pin toggles) | Planned (mentioned in design.md) |
| **Data usage** | Notecard byte counters | Planned |
