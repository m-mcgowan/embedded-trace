# Changelog

All notable changes to this project will be documented in this file.
Follows [Keep a Changelog](https://keepachangelog.com/) conventions.

## [Unreleased]

### Added
- ITracer interface — abstract base for all tracer implementations
- ScopeGuard — RAII scope entry/exit tracing with automatic lifetime management
- ScopeGuard::end() — explicit close for use before no-return calls (deep sleep, restart, abort)
- esp_idf_tid_fn() helper — FreeRTOS task handle as ThreadId; SerialTracer uses it as the default tid_fn on ESP-IDF (one Perfetto lane per task instead of single-lane)
- ITracer::set_process_name() / set_thread_name() — emit Chrome ph:M metadata events. SerialTracer renders them as JSON; CompositeTracer forwards to all children; NullTracer/BufferTracer no-op (default).
- examples/basic_scopes — host-runnable native example exercising scope nesting, counters, and metadata events with SerialTracer to stdout.
- examples/esp32_perfetto — ESP32-S3 Arduino app emitting Chrome JSON over USB Serial, with two FreeRTOS tasks demonstrating one-Perfetto-lane-per-task via the new esp_idf_tid_fn default.
- README quickstart — install instructions, copy-paste-runnable code sample, and Chrome JSON output preview, replacing the previous status-table-only README.
- Build-time #warning when SerialTracer / BufferTracer / CompositeTracer headers are included without -DEMBEDDED_TRACE_ENABLED=1, catching the silent-no-op-macro footgun at compile time.

### Changed
- **ITracer::scope() gains an optional category parameter.** Signature: `scope(const char* cat_or_name, const char* name = nullptr)`. Two forms: `scope("notecard", "verify")` (explicit) and `scope("notecard.verify")` (dotted, auto-split at render time). Emits Chrome "cat" field on B and E events — unlocks Perfetto colour-coding and SQL filtering. Shared split rule lives in `embedded_trace/name_split.h`. BufferTracer's DrainEvent gains a `cat` field; scope-name interning is now by `(cat, name)` pair, costing +4 bytes per entry (~256 bytes on ESP32 with 64-entry default).
- **ScopeGuard::ExitFn signature gains a `cat` parameter** to thread explicit categories through to the exit event. ScopeGuard grows by one pointer (~4 bytes on 32-bit). Migration: existing ExitFn callbacks take one extra `const char* cat` argument after `void* context`.
- **ITracer is now fully pure virtual.** flow_start/step/end and set_process_name/set_thread_name were previously virtual-with-empty-bodies, which silently dropped events on any subclass that forgot to override. Implementations now either override every method explicitly or inherit from a narrow named mixin:
  - `NoFlowTracer` — no-op defaults for the three flow_* methods
  - `NoMetadataTracer` — no-op defaults for the two set_*_name methods
  No generic `BaseTracer` catch-all — pick by what you're opting out of.
- NullTracer now implements all seven methods inline (the canonical "do nothing" reference). BufferTracer now inherits NoMetadataTracer. SerialTracer and CompositeTracer unchanged — they already implement the full contract. Third-party tracer subclasses will get a compile error pointing at the missing method, which is the point.

### Documentation
- design.md "Tracer ownership" section: three patterns (injected, user-declared global reference, FreeRTOS task-local storage) with their catches. Library ships no global / TLS helper — each pattern is user-side. Notes the C++ thread_local emutls hazard on ESP-IDF (idle task stack overflow) and points at vTaskSetThreadLocalStoragePointer instead.
- Scope name lifetime rule spelled out: names must be string literals (BufferTracer interns by pointer equality; SerialTracer is lenient but portable code must still use literals). Noted in ITracer::scope() doc, design.md, and README Gotchas.
- README Gotchas expanded: SerialTracer's 192-byte stack buffer silently truncates names > ~160 chars; TimestampUs wraps at ~71 min; four naming forms (embedded-trace / embedded_trace / et / ET_) called out.
- ScopeGuard default constructor intent documented (no-op guard; also the state after move-from / end()).
- README Install clarifies that both srcFilter dirs compile cleanly on native (bindings guard FreeRTOS/Arduino includes under ESP_PLATFORM / ARDUINO) — no build_src_filter gymnastics required.
- SerialTracer class docstring notes the 192-byte buffer limit.
- NullTracer — zero-cost no-op tracer for production builds
- SerialTracer — ESP32 serial tracer emitting Chrome Trace Event JSON
- Trace macros (TRACE_SCOPE, TRACE_COUNTER) for convenient instrumentation
- Native host tests for ScopeGuard and SerialTracer (doctest)
- Design document covering architecture, threading model, and future causal profiling
