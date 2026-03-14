#include "buffer_tracer.h"
#include <cstring>

namespace et {

BufferTracer::BufferTracer(uint8_t* buffer, size_t size, TimestampFn timestamp_fn)
    : buffer_(buffer), buffer_size_(size), timestamp_fn_(timestamp_fn),
      write_pos_(0), event_count_(0), overflowed_(false),
      scope_names_{}, scope_name_count_(0) {}

ScopeId BufferTracer::intern_name(const char* name) {
    // Search by pointer equality (scope names must be string literals)
    for (ScopeId i = 0; i < scope_name_count_; ++i) {
        if (scope_names_[i] == name) {
            return i;
        }
    }
    // Add new name if space available
    if (scope_name_count_ < MAX_SCOPE_NAMES) {
        ScopeId id = scope_name_count_++;
        scope_names_[id] = name;
        return id;
    }
    // Table full — reuse last slot (degenerate case)
    return static_cast<ScopeId>(MAX_SCOPE_NAMES - 1);
}

bool BufferTracer::write_event(EventType type, ScopeId scope_id,
                               const void* payload, size_t payload_size) {
    // Event format: [timestamp:4][scope_id:2][type:1][payload:0-8]
    size_t event_size = 7 + payload_size;
    if (write_pos_ + event_size > buffer_size_) {
        overflowed_ = true;
        return false;
    }

    TimestampUs ts = timestamp_fn_();
    uint8_t* p = buffer_ + write_pos_;

    // Timestamp (4 bytes, little-endian)
    p[0] = static_cast<uint8_t>(ts);
    p[1] = static_cast<uint8_t>(ts >> 8);
    p[2] = static_cast<uint8_t>(ts >> 16);
    p[3] = static_cast<uint8_t>(ts >> 24);

    // Scope ID (2 bytes, little-endian)
    p[4] = static_cast<uint8_t>(scope_id);
    p[5] = static_cast<uint8_t>(scope_id >> 8);

    // Event type (1 byte)
    p[6] = static_cast<uint8_t>(type);

    // Payload
    if (payload && payload_size > 0) {
        memcpy(p + 7, payload, payload_size);
    }

    write_pos_ += event_size;
    event_count_++;
    return true;
}

ScopeGuard BufferTracer::scope(const char* name) {
    ScopeId id = intern_name(name);
    write_event(EventType::scope_enter, id);
    return ScopeGuard(this, scope_exit_callback, name, id);
}

void BufferTracer::counter(const char* name, int64_t value) {
    ScopeId id = intern_name(name);
    write_event(EventType::counter, id, &value, sizeof(value));
}

void BufferTracer::flow_start(const char* name, FlowId id) {
    ScopeId scope_id = intern_name(name);
    write_event(EventType::flow_start, scope_id, &id, sizeof(id));
}

void BufferTracer::flow_step(const char* name, FlowId id) {
    ScopeId scope_id = intern_name(name);
    write_event(EventType::flow_step, scope_id, &id, sizeof(id));
}

void BufferTracer::flow_end(const char* name, FlowId id) {
    ScopeId scope_id = intern_name(name);
    write_event(EventType::flow_end, scope_id, &id, sizeof(id));
}

void BufferTracer::drain(DrainCallback callback, void* context) const {
    size_t pos = 0;
    while (pos < write_pos_) {
        if (pos + 7 > write_pos_) break;

        uint8_t const* p = buffer_ + pos;

        // Timestamp (4 bytes, little-endian)
        TimestampUs ts = static_cast<TimestampUs>(p[0])
                       | (static_cast<TimestampUs>(p[1]) << 8)
                       | (static_cast<TimestampUs>(p[2]) << 16)
                       | (static_cast<TimestampUs>(p[3]) << 24);

        // Scope ID (2 bytes, little-endian)
        ScopeId scope_id = static_cast<ScopeId>(p[4])
                         | (static_cast<ScopeId>(p[5]) << 8);

        // Event type
        auto type = static_cast<EventType>(p[6]);

        DrainEvent event{};
        event.timestamp = ts;
        event.type = type;
        event.name = (scope_id < scope_name_count_) ? scope_names_[scope_id] : "";

        size_t payload_size = 0;

        switch (type) {
        case EventType::scope_enter:
        case EventType::scope_exit:
            break;
        case EventType::counter:
            payload_size = sizeof(int64_t);
            if (pos + 7 + payload_size <= write_pos_) {
                memcpy(&event.value, p + 7, sizeof(int64_t));
            }
            break;
        case EventType::flow_start:
        case EventType::flow_step:
        case EventType::flow_end:
            payload_size = sizeof(FlowId);
            if (pos + 7 + payload_size <= write_pos_) {
                memcpy(&event.flow_id, p + 7, sizeof(FlowId));
            }
            break;
        }

        callback(context, event);
        pos += 7 + payload_size;
    }
}

void BufferTracer::reset() {
    write_pos_ = 0;
    event_count_ = 0;
    overflowed_ = false;
    // Keep scope name table — names are still valid string literals
}

void BufferTracer::scope_exit_callback(void* ctx, const char* name, ScopeId scope_id) {
    auto* self = static_cast<BufferTracer*>(ctx);
    self->write_event(EventType::scope_exit, scope_id);
}

} // namespace et
