# embedded-trace

Lightweight, hierarchical scope-tracing library for embedded systems. Emits Chrome Trace Event JSON (loadable in [Perfetto](https://ui.perfetto.dev)) from RAII-scoped instrumentation. Designed for ESP32/FreeRTOS as a first target, with a platform-agnostic core that compiles on any C++17 host.

## Quickstart

```cpp
#include <embedded_trace/trace_macros.h>
#include <embedded_trace_esp32/serial_tracer.h>

et::SerialTracer tracer(Serial, micros);  // micros() — your timestamp source

void setup() {
    Serial.begin(115200);
    tracer.set_process_name("my_app");
    tracer.set_thread_name(et::esp_idf_tid_fn(), "loopTask");
}

void loop() {
    TRACE_SCOPE(tracer, "cycle");
    {
        TRACE_SCOPE(tracer, "work");
        do_thing();
    }
    TRACE_COUNTER(tracer, "heap.free", ESP.getFreeHeap());
    delay(100);
}
```

Output is one Chrome JSON event per line on `Serial`:

```json
{"ph":"M","name":"process_name","pid":1,"args":{"name":"my_app"}}
{"ph":"B","ts":1234,"name":"cycle","pid":1,"tid":1073553228}
{"ph":"E","ts":2345,"name":"work","pid":1,"tid":1073553228}
{"ph":"C","ts":2346,"name":"heap.free","pid":1,"tid":1073553228,"args":{"value":243000}}
```

Wrap captured lines as `{"traceEvents":[...]}` and load in [https://ui.perfetto.dev](https://ui.perfetto.dev).

## Install

PlatformIO:

```ini
lib_deps = https://github.com/m-mcgowan/embedded-trace.git
build_flags = -DEMBEDDED_TRACE_ENABLED=1
```

Without `-DEMBEDDED_TRACE_ENABLED=1`, all `TRACE_*` macros compile to no-ops (zero code, zero RAM, zero CPU). The tracer headers emit a `#warning` if you include them with the flag off — a common first-time-user trip.

`library.json`'s `srcFilter` compiles both `embedded_trace/` (core) and `embedded_trace_esp32/` (ESP-IDF bindings). The bindings guard their FreeRTOS/Arduino includes behind `#ifdef ESP_PLATFORM` / `#ifdef ARDUINO`, so the whole library compiles cleanly on the `native` platform too — no `build_src_filter` gymnastics required.

## Examples

- [`examples/basic_scopes/`](examples/basic_scopes/) — host-runnable native demo. `pio run -e basic_scopes -t exec`.
- [`examples/esp32_perfetto/`](examples/esp32_perfetto/) — ESP32-S3 + multi-task Perfetto traces. `pio run -e esp32_perfetto -t upload`.

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
- **Scope names must be string literals.** `BufferTracer` interns by
  pointer equality; passing `std::string::c_str()` or a stack buffer
  breaks drain. `SerialTracer` is lenient but code portable to both must
  use literals. See [design.md — Scope name lifetime](docs/design.md#scope-name-lifetime).
- **Scope names longer than ~160 chars silently truncate** in `SerialTracer`'s
  192-byte stack buffer, producing malformed JSON. Keep names short.
- **Timestamps wrap at ~71 minutes.** `TimestampUs` is `uint32_t`; for
  long-running traces, wrap handling is the host collector's job.
- **Naming conventions:** the package is `embedded-trace` (hyphen) in
  `lib_deps`; the headers are under `embedded_trace/` (underscore); the
  namespace is `et::`; the macro prefix is `ET_`. Four forms of the
  same name — copy-paste carefully.

## Further reading

[`docs/design.md`](docs/design.md) — full architecture, threading model, BufferTracer + CompositeTracer, cross-thread causal profiling.
