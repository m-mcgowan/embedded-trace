#pragma once

#include "../embedded_trace/i_tracer.h"
#include "../embedded_trace/types.h"

#ifdef ARDUINO
#include <Print.h>
#else
#include "../embedded_trace/native_print.h"
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
 * Output is directly loadable in Perfetto UI when wrapped by the host
 * in {"traceEvents":[...]}.
 */
class SerialTracer final : public ITracer {
public:
    /**
     * @param output       Print stream (e.g. Serial)
     * @param timestamp_fn Microsecond timestamp source
     * @param pid          Process ID for Chrome JSON (default 1)
     * @param tid_fn       Thread ID function. When null:
     *                       - on ESP-IDF (ESP_PLATFORM): defaults to
     *                         et::esp_idf_tid_fn (one Perfetto lane per
     *                         FreeRTOS task)
     *                       - elsewhere: defaults to constant 1
     */
    SerialTracer(Print& output, TimestampFn timestamp_fn,
                 ProcessId pid = 1, ThreadIdFn tid_fn = nullptr);

    ScopeGuard scope(const char* name) override;
    void counter(const char* name, int64_t value) override;
    void flow_start(const char* name, FlowId id) override;
    void flow_step(const char* name, FlowId id) override;
    void flow_end(const char* name, FlowId id) override;
    void set_process_name(const char* name) override;
    void set_thread_name(ThreadId tid, const char* name) override;

private:
    Print& output_;
    TimestampFn timestamp_fn_;
    ProcessId pid_;
    ThreadIdFn tid_fn_;

    void emit_begin(const char* name);
    void emit_end(const char* name, ScopeId scope_id);

    static void scope_exit_callback(void* context, const char* name, ScopeId scope_id);
};

} // namespace et
