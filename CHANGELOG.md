# Changelog

All notable changes to this project will be documented in this file.
Follows [Keep a Changelog](https://keepachangelog.com/) conventions.

## [Unreleased]

### Added
- ITracer interface — abstract base for all tracer implementations
- ScopeGuard — RAII scope entry/exit tracing with automatic lifetime management
- ScopeGuard::end() — explicit close for use before no-return calls (deep sleep, restart, abort)
- NullTracer — zero-cost no-op tracer for production builds
- SerialTracer — ESP32 serial tracer emitting Chrome Trace Event JSON
- Trace macros (TRACE_SCOPE, TRACE_COUNTER) for convenient instrumentation
- Native host tests for ScopeGuard and SerialTracer (doctest)
- Design document covering architecture, threading model, and future causal profiling
