#pragma once

/**
 * @brief Zero-cost trace macros.
 *
 * When EMBEDDED_TRACE_ENABLED is 0 or undefined, both macros compile
 * to nothing — zero code size, zero RAM, zero CPU.
 */

// Concatenation helpers for unique variable names
#define ET_CONCAT_(a, b) a##b
#define ET_CONCAT(a, b) ET_CONCAT_(a, b)

#if EMBEDDED_TRACE_ENABLED
  #define TRACE_SCOPE(tracer, name) \
      auto ET_CONCAT(_et_scope_, __LINE__) = (tracer).scope(name)
  #define TRACE_COUNTER(tracer, name, val) \
      (tracer).counter(name, val)
  #define TRACE_FLOW_START(tracer, name, id) \
      (tracer).flow_start(name, id)
  #define TRACE_FLOW_STEP(tracer, name, id) \
      (tracer).flow_step(name, id)
  #define TRACE_FLOW_END(tracer, name, id) \
      (tracer).flow_end(name, id)
#else
  #define TRACE_SCOPE(tracer, name) ((void)0)
  #define TRACE_COUNTER(tracer, name, val) ((void)0)
  #define TRACE_FLOW_START(tracer, name, id) ((void)0)
  #define TRACE_FLOW_STEP(tracer, name, id) ((void)0)
  #define TRACE_FLOW_END(tracer, name, id) ((void)0)
#endif
