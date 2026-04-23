# Changelog

All notable changes to this project will be documented in this file.
Follows [Keep a Changelog](https://keepachangelog.com/) conventions.

## [Unreleased]

### Added
- ITracer interface — abstract base for all tracer implementations
- ScopeGuard — RAII scope entry/exit tracing with automatic lifetime management
- ScopeGuard::end() — explicit close for use before no-return calls (deep sleep, restart, abort)
- esp_idf_tid_fn() helper — FreeRTOS task handle as ThreadId; SerialTracer uses it as the default tid_fn on ESP-IDF (one Perfetto lane per task instead of single-lane)
- NullTracer — zero-cost no-op tracer for production builds
- SerialTracer — ESP32 serial tracer emitting Chrome Trace Event JSON
- Trace macros (TRACE_SCOPE, TRACE_COUNTER) for convenient instrumentation
- Native host tests for ScopeGuard and SerialTracer (doctest)
- Design document covering architecture, threading model, and future causal profiling
