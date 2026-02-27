#pragma once

#include "../embedded_tracer/i_tracer.h"
#include "../embedded_tracer/types.h"

#ifdef ARDUINO
#include <Print.h>
#else
#include "../embedded_tracer/native_print.h"
#endif

namespace et {

/**
 * @brief Chrome JSON tracer that emits to a Print stream (e.g. Serial).
 *
 * Each scope enter/exit emits one JSON line:
 *   {"ph":"B","ts":1234,"name":"gps_fix","pid":1,"tid":1}
 *   {"ph":"E","ts":5678,"name":"gps_fix","pid":1,"tid":1}
 *
 * Counters emit:
 *   {"ph":"C","ts":5678,"name":"heap","pid":1,"args":{"value":102400}}
 *
 * When ppk2_markers is true, also emits T= event markers:
 *   T=0.001234 GPS_FIX_STARTED
 *   T=0.005678 GPS_FIX_STOPPED
 *
 * Output is directly loadable in Perfetto UI when wrapped by the host
 * in {"traceEvents":[...]}.
 */
class SerialTracer final : public ITracer {
public:
    /**
     * @param output       Print stream (e.g. Serial)
     * @param timestamp_fn Microsecond timestamp source
     * @param ppk2_markers If true, emit T= event markers alongside JSON
     * @param pid          Process ID for Chrome JSON (default 1)
     */
    SerialTracer(Print& output, TimestampFn timestamp_fn,
                 bool ppk2_markers = false, ProcessId pid = 1);

    ScopeGuard scope(const char* name) override;
    void counter(const char* name, int64_t value) override;

private:
    Print& output_;
    TimestampFn timestamp_fn_;
    bool ppk2_markers_;
    ProcessId pid_;

    void emit_begin(const char* name);
    void emit_end(const char* name, ScopeId scope_id);

    static void scope_exit_callback(void* context, const char* name, ScopeId scope_id);
};

} // namespace et
