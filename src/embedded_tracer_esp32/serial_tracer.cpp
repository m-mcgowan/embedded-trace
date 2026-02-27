#include "serial_tracer.h"
#include <cstdio>
#include <cstring>

namespace et {

SerialTracer::SerialTracer(Print& output, TimestampFn timestamp_fn,
                           bool ppk2_markers, ProcessId pid)
    : output_(output), timestamp_fn_(timestamp_fn),
      ppk2_markers_(ppk2_markers), pid_(pid) {}

void SerialTracer::emit_begin(const char* name) {
    TimestampUs ts = timestamp_fn_();

    if (ppk2_markers_) {
        char marker[128];
        char upper_name[64];
        size_t i = 0;
        for (const char* p = name; *p && i < sizeof(upper_name) - 1; ++p, ++i) {
            upper_name[i] = (*p >= 'a' && *p <= 'z') ? (*p - 32) : *p;
        }
        upper_name[i] = '\0';
        snprintf(marker, sizeof(marker), "T=%u.%06u %s_STARTED",
                 ts / 1000000, ts % 1000000, upper_name);
        output_.println(marker);
    }

    char json[192];
    snprintf(json, sizeof(json),
             "{\"ph\":\"B\",\"ts\":%u,\"name\":\"%s\",\"pid\":%u,\"tid\":1}",
             ts, name, pid_);
    output_.println(json);
}

void SerialTracer::emit_end(const char* name, ScopeId /*scope_id*/) {
    TimestampUs ts = timestamp_fn_();

    if (ppk2_markers_) {
        char marker[128];
        char upper_name[64];
        size_t i = 0;
        for (const char* p = name; *p && i < sizeof(upper_name) - 1; ++p, ++i) {
            upper_name[i] = (*p >= 'a' && *p <= 'z') ? (*p - 32) : *p;
        }
        upper_name[i] = '\0';
        snprintf(marker, sizeof(marker), "T=%u.%06u %s_STOPPED",
                 ts / 1000000, ts % 1000000, upper_name);
        output_.println(marker);
    }

    char json[192];
    snprintf(json, sizeof(json),
             "{\"ph\":\"E\",\"ts\":%u,\"name\":\"%s\",\"pid\":%u,\"tid\":1}",
             ts, name, pid_);
    output_.println(json);
}

ScopeGuard SerialTracer::scope(const char* name) {
    emit_begin(name);
    return ScopeGuard(this, scope_exit_callback, name, 0);
}

void SerialTracer::counter(const char* name, int64_t value) {
    TimestampUs ts = timestamp_fn_();
    char json[192];
    snprintf(json, sizeof(json),
             "{\"ph\":\"C\",\"ts\":%u,\"name\":\"%s\",\"pid\":%u,\"args\":{\"value\":%lld}}",
             ts, name, pid_, (long long)value);
    output_.println(json);
}

void SerialTracer::scope_exit_callback(void* context, const char* name, ScopeId scope_id) {
    auto* self = static_cast<SerialTracer*>(context);
    self->emit_end(name, scope_id);
}

} // namespace et
