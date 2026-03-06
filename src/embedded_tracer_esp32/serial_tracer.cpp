#include "serial_tracer.h"
#include <cstdio>

namespace et {

static ThreadId default_tid() { return 1; }

SerialTracer::SerialTracer(Print& output, TimestampFn timestamp_fn,
                           ProcessId pid, ThreadIdFn tid_fn)
    : output_(output), timestamp_fn_(timestamp_fn),
      pid_(pid), tid_fn_(tid_fn ? tid_fn : default_tid) {}

void SerialTracer::emit_begin(const char* name) {
    TimestampUs ts = timestamp_fn_();
    ThreadId tid = tid_fn_();
    char json[192];
    snprintf(json, sizeof(json),
             "{\"ph\":\"B\",\"ts\":%u,\"name\":\"%s\",\"pid\":%u,\"tid\":%u}",
             ts, name, pid_, tid);
    output_.println(json);
}

void SerialTracer::emit_end(const char* name, ScopeId /*scope_id*/) {
    TimestampUs ts = timestamp_fn_();
    ThreadId tid = tid_fn_();
    char json[192];
    snprintf(json, sizeof(json),
             "{\"ph\":\"E\",\"ts\":%u,\"name\":\"%s\",\"pid\":%u,\"tid\":%u}",
             ts, name, pid_, tid);
    output_.println(json);
}

ScopeGuard SerialTracer::scope(const char* name) {
    emit_begin(name);
    return ScopeGuard(this, scope_exit_callback, name, 0);
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

void SerialTracer::scope_exit_callback(void* context, const char* name, ScopeId scope_id) {
    auto* self = static_cast<SerialTracer*>(context);
    self->emit_end(name, scope_id);
}

} // namespace et
