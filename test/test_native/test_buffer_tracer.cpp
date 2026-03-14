#include <doctest/doctest.h>
#include <embedded_trace/buffer_tracer.h>
#include <vector>

namespace et {

// ── Mock timestamp ───────────────────────────────────────────────

static TimestampUs buf_mock_time = 0;
static TimestampUs buf_mock_timestamp() { return buf_mock_time; }

// ── Drain helper ─────────────────────────────────────────────────

struct CollectedEvent {
    TimestampUs timestamp;
    const char* name;
    EventType type;
    int64_t value;
    FlowId flow_id;
};

static void collect_callback(void* ctx, const DrainEvent& event) {
    auto* vec = static_cast<std::vector<CollectedEvent>*>(ctx);
    vec->push_back({event.timestamp, event.name, event.type, event.value, event.flow_id});
}

static std::vector<CollectedEvent> drain_all(const BufferTracer& tracer) {
    std::vector<CollectedEvent> events;
    tracer.drain(collect_callback, &events);
    return events;
}

// ── Basic scope events ──────────────────────────────────────────

TEST_CASE("BufferTracer: scope records enter and exit") {
    uint8_t buf[256];
    buf_mock_time = 1000;
    BufferTracer tracer(buf, sizeof(buf), buf_mock_timestamp);

    {
        auto guard = tracer.scope("gps_fix");
        CHECK(tracer.event_count() == 1);
        buf_mock_time = 5000;
    }
    CHECK(tracer.event_count() == 2);

    auto events = drain_all(tracer);
    REQUIRE(events.size() == 2);
    CHECK(events[0].type == EventType::scope_enter);
    CHECK(events[0].timestamp == 1000);
    CHECK(std::string(events[0].name) == "gps_fix");
    CHECK(events[1].type == EventType::scope_exit);
    CHECK(events[1].timestamp == 5000);
}

TEST_CASE("BufferTracer: counter records value") {
    uint8_t buf[256];
    buf_mock_time = 2000;
    BufferTracer tracer(buf, sizeof(buf), buf_mock_timestamp);

    tracer.counter("heap_free", 102400);
    CHECK(tracer.event_count() == 1);

    auto events = drain_all(tracer);
    REQUIRE(events.size() == 1);
    CHECK(events[0].type == EventType::counter);
    CHECK(events[0].value == 102400);
    CHECK(std::string(events[0].name) == "heap_free");
}

TEST_CASE("BufferTracer: nested scopes") {
    uint8_t buf[256];
    buf_mock_time = 0;
    BufferTracer tracer(buf, sizeof(buf), buf_mock_timestamp);

    {
        buf_mock_time = 100;
        auto outer = tracer.scope("outer");
        {
            buf_mock_time = 200;
            auto inner = tracer.scope("inner");
            buf_mock_time = 300;
        }
        buf_mock_time = 400;
    }

    auto events = drain_all(tracer);
    REQUIRE(events.size() == 4);
    CHECK(events[0].type == EventType::scope_enter);   // outer enter
    CHECK(events[0].timestamp == 100);
    CHECK(events[1].type == EventType::scope_enter);   // inner enter
    CHECK(events[1].timestamp == 200);
    CHECK(events[2].type == EventType::scope_exit);    // inner exit
    CHECK(events[2].timestamp == 300);
    CHECK(events[3].type == EventType::scope_exit);    // outer exit
    CHECK(events[3].timestamp == 400);
}

// ── Flow events ─────────────────────────────────────────────────

TEST_CASE("BufferTracer: flow events") {
    uint8_t buf[256];
    buf_mock_time = 1000;
    BufferTracer tracer(buf, sizeof(buf), buf_mock_timestamp);

    tracer.flow_start("req", 42);
    buf_mock_time = 2000;
    tracer.flow_step("req", 42);
    buf_mock_time = 3000;
    tracer.flow_end("req", 42);

    auto events = drain_all(tracer);
    REQUIRE(events.size() == 3);
    CHECK(events[0].type == EventType::flow_start);
    CHECK(events[0].flow_id == 42);
    CHECK(events[1].type == EventType::flow_step);
    CHECK(events[2].type == EventType::flow_end);
}

// ── Overflow ────────────────────────────────────────────────────

TEST_CASE("BufferTracer: overflow detection") {
    uint8_t buf[16];  // Only room for 2 scope events (7 bytes each) + 2 spare bytes
    buf_mock_time = 0;
    BufferTracer tracer(buf, sizeof(buf), buf_mock_timestamp);

    CHECK_FALSE(tracer.overflowed());

    // First scope enter: 7 bytes, fits
    {
        auto guard = tracer.scope("a");
        CHECK(tracer.event_count() == 1);
        CHECK_FALSE(tracer.overflowed());
        // Exit will write 7 more bytes (total 14), fits in 16
    }
    CHECK(tracer.event_count() == 2);
    CHECK_FALSE(tracer.overflowed());

    // Third event: 7 bytes would need pos 14+7=21 > 16
    tracer.counter("x", 1);  // 15 bytes, won't fit
    CHECK(tracer.overflowed());
    CHECK(tracer.event_count() == 2);  // didn't increase
}

// ── Reset ───────────────────────────────────────────────────────

TEST_CASE("BufferTracer: reset clears events but keeps name table") {
    uint8_t buf[256];
    buf_mock_time = 0;
    BufferTracer tracer(buf, sizeof(buf), buf_mock_timestamp);

    {
        auto guard = tracer.scope("test");
    }
    CHECK(tracer.event_count() == 2);

    tracer.reset();
    CHECK(tracer.event_count() == 0);
    CHECK_FALSE(tracer.overflowed());

    // Can still use same scope name after reset
    buf_mock_time = 100;
    {
        auto guard = tracer.scope("test");
        buf_mock_time = 200;
    }

    auto events = drain_all(tracer);
    REQUIRE(events.size() == 2);
    CHECK(std::string(events[0].name) == "test");
}

// ── Scope name interning ────────────────────────────────────────

TEST_CASE("BufferTracer: same name pointer shares scope ID") {
    uint8_t buf[256];
    buf_mock_time = 0;
    BufferTracer tracer(buf, sizeof(buf), buf_mock_timestamp);

    {
        auto g1 = tracer.scope("shared");
        buf_mock_time = 1;
    }
    buf_mock_time = 2;
    {
        auto g2 = tracer.scope("shared");
        buf_mock_time = 3;
    }

    auto events = drain_all(tracer);
    REQUIRE(events.size() == 4);
    // All events should resolve to the same name
    for (auto& e : events) {
        CHECK(std::string(e.name) == "shared");
    }
}

} // namespace et
