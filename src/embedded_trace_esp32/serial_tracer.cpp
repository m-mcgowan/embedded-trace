#include "serial_tracer.h"
#include "esp_idf_tid_fn.h"
#include "../embedded_trace/name_split.h"
#include <cstdio>

namespace et {

static ThreadId default_tid_fn() {
#ifdef ESP_PLATFORM
    return esp_idf_tid_fn();
#else
    return 1;
#endif
}

SerialTracer::SerialTracer(Print& output, TimestampFn timestamp_fn,
                           ProcessId pid, ThreadIdFn tid_fn)
    : output_(output), timestamp_fn_(timestamp_fn),
      pid_(pid), tid_fn_(tid_fn ? tid_fn : default_tid_fn) {}

// Render B or E with optional cat. phase is 'B' or 'E'. When cat is
// null, name may be a dotted identifier — split on first dot at render
// time. Explicit-cat case passes cat non-null and name verbatim.
void SerialTracer::emit_scope_event(char phase, const char* cat, const char* name) {
    TimestampUs ts = timestamp_fn_();
    ThreadId tid = tid_fn_();
    char json[192];
    if (cat) {
        // Explicit cat (no splitting). name used verbatim.
        snprintf(json, sizeof(json),
                 "{\"ph\":\"%c\",\"ts\":%u,\"cat\":\"%s\",\"name\":\"%s\",\"pid\":%u,\"tid\":%u}",
                 phase, ts, cat, name, pid_, tid);
    } else {
        // Single-arg form — auto-split on first dot.
        SplitName s = split_scope_name(name, nullptr);
        if (s.cat) {
            snprintf(json, sizeof(json),
                     "{\"ph\":\"%c\",\"ts\":%u,\"cat\":\"%.*s\",\"name\":\"%s\",\"pid\":%u,\"tid\":%u}",
                     phase, ts, (int)s.cat_len, s.cat, s.name, pid_, tid);
        } else {
            snprintf(json, sizeof(json),
                     "{\"ph\":\"%c\",\"ts\":%u,\"name\":\"%s\",\"pid\":%u,\"tid\":%u}",
                     phase, ts, (name ? name : ""), pid_, tid);
        }
    }
    output_.println(json);
}

void SerialTracer::emit_begin(const char* cat, const char* name) {
    emit_scope_event('B', cat, name);
}

void SerialTracer::emit_end(const char* cat, const char* name) {
    emit_scope_event('E', cat, name);
}

ScopeGuard SerialTracer::scope(const char* cat_or_name, const char* name) {
    // Explicit form: scope(cat, name) — cat non-null, name non-null.
    // Single-arg form: scope(name_or_dotted) — first param is the full
    // identifier, second is null; emit_scope_event() splits at render.
    const char* cat = name ? cat_or_name : nullptr;
    const char* scope_name = name ? name : cat_or_name;
    emit_begin(cat, scope_name);
    return ScopeGuard(this, scope_exit_callback, cat, scope_name, 0);
}

void SerialTracer::counter(const char* name, int64_t value) {
    TimestampUs ts = timestamp_fn_();
    ThreadId tid = tid_fn_();
    char json[192];
    snprintf(json, sizeof(json),
             "{\"ph\":\"C\",\"ts\":%u,\"name\":\"%s\",\"pid\":%u,\"tid\":%u,\"args\":{\"value\":%lld}}",
             ts, name, pid_, tid, (long long)value);
    output_.println(json);
}

void SerialTracer::flow_start(const char* name, FlowId id) {
    TimestampUs ts = timestamp_fn_();
    ThreadId tid = tid_fn_();
    char json[192];
    snprintf(json, sizeof(json),
             "{\"ph\":\"s\",\"ts\":%u,\"name\":\"%s\",\"id\":%u,\"pid\":%u,\"tid\":%u}",
             ts, name, (unsigned)id, pid_, tid);
    output_.println(json);
}

void SerialTracer::flow_step(const char* name, FlowId id) {
    TimestampUs ts = timestamp_fn_();
    ThreadId tid = tid_fn_();
    char json[192];
    snprintf(json, sizeof(json),
             "{\"ph\":\"t\",\"ts\":%u,\"name\":\"%s\",\"id\":%u,\"pid\":%u,\"tid\":%u}",
             ts, name, (unsigned)id, pid_, tid);
    output_.println(json);
}

void SerialTracer::flow_end(const char* name, FlowId id) {
    TimestampUs ts = timestamp_fn_();
    ThreadId tid = tid_fn_();
    char json[192];
    snprintf(json, sizeof(json),
             "{\"ph\":\"f\",\"ts\":%u,\"name\":\"%s\",\"id\":%u,\"pid\":%u,\"tid\":%u}",
             ts, name, (unsigned)id, pid_, tid);
    output_.println(json);
}

void SerialTracer::scope_exit_callback(void* context, const char* cat,
                                       const char* name, ScopeId /*scope_id*/) {
    auto* self = static_cast<SerialTracer*>(context);
    self->emit_end(cat, name);
}

void SerialTracer::set_process_name(const char* name) {
    char json[192];
    snprintf(json, sizeof(json),
             "{\"ph\":\"M\",\"name\":\"process_name\",\"pid\":%u,\"args\":{\"name\":\"%s\"}}",
             pid_, name);
    output_.println(json);
}

void SerialTracer::set_thread_name(ThreadId tid, const char* name) {
    char json[192];
    snprintf(json, sizeof(json),
             "{\"ph\":\"M\",\"name\":\"thread_name\",\"pid\":%u,\"tid\":%u,\"args\":{\"name\":\"%s\"}}",
             pid_, tid, name);
    output_.println(json);
}

} // namespace et
