# embedded-trace

Lightweight, hierarchical scope-tracing library for embedded systems. Designed for ESP32/FreeRTOS as a first target, with a platform-agnostic core that compiles on any C++17 host.

See [docs/design.md](docs/design.md) for architecture and design.

## Status

| Feature | Design | Docs | Impl | Tests | Examples | Since | Updated |
|---------|--------|------|------|-------|----------|-------|---------|
| **ITracer interface** | [design.md](docs/design.md) | | [i_tracer.h](src/embedded_trace/i_tracer.h) | | | | |
| **ScopeGuard (RAII tracing)** | [design.md](docs/design.md) | | [scope_guard.h](src/embedded_trace/scope_guard.h) | [test_scope_guard](test/test_native/test_scope_guard.cpp) | | | |
| **NullTracer (no-op production)** | [design.md](docs/design.md) | | [null_tracer.h](src/embedded_trace/null_tracer.h) | | | | |
| **SerialTracer (Chrome JSON)** | [design.md](docs/design.md) | | [serial_tracer.h](src/embedded_trace_esp32/serial_tracer.h) | [test_serial_tracer](test/test_native/test_serial_tracer.cpp) | | | |
| **Trace macros** | [design.md](docs/design.md) | | [trace_macros.h](src/embedded_trace/trace_macros.h) | | | | |
