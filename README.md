# embedded-trace

Lightweight, hierarchical scope-tracing library for embedded systems. Designed for ESP32/FreeRTOS as a first target, with a platform-agnostic core that compiles on any C++17 host.

See [docs/design.md](docs/design.md) for architecture and design.

## Status

| Feature | Design | Docs | Impl | Tests | Examples | Since | Updated |
|---------|--------|------|------|-------|----------|-------|---------|
| **ITracer interface** | [design.md](docs/design.md) | | [i_tracer.h](src/embedded_trace/i_tracer.h) | | | | |
| **ScopeGuard (RAII tracing)** | [design.md](docs/design.md) | | [scope_guard.h](src/embedded_trace/scope_guard.h) | [test_scope_guard](test/test_native/test_scope_guard.cpp) | [basic_scopes](examples/basic_scopes/main.cpp) | | |
| **NullTracer (no-op production)** | [design.md](docs/design.md) | | [null_tracer.h](src/embedded_trace/null_tracer.h) | | | | |
| **SerialTracer (Chrome JSON)** | [design.md](docs/design.md) | | [serial_tracer.h](src/embedded_trace_esp32/serial_tracer.h) | [test_serial_tracer](test/test_native/test_serial_tracer.cpp) | [esp32_perfetto](examples/esp32_perfetto/main.cpp) | | |
| **Trace macros** | [design.md](docs/design.md) | | [trace_macros.h](src/embedded_trace/trace_macros.h) | | [basic_scopes](examples/basic_scopes/main.cpp) | | |

## Gotchas

- **No-return calls (`esp_deep_sleep_start`, `esp_restart`, `abort`)** skip
  stack unwinding, so any live `TRACE_SCOPE` emits `B` with no matching `E`.
  Pop back above the scope before the no-return call, or hold the guard
  manually (`auto g = tracer.scope(...)`) and call `g.end()` before it.
  See [design.md — Scopes and no-return calls](docs/design.md#scopes-and-no-return-calls).
