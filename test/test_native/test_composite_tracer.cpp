#include <doctest/doctest.h>
#include <embedded_tracer/composite_tracer.h>
#include <embedded_tracer/buffer_tracer.h>
#include <embedded_tracer/null_tracer.h>
#include <vector>
#include <string>

namespace et {

// ── Mock timestamp ───────────────────────────────────────────────

static TimestampUs comp_mock_time = 0;
static TimestampUs comp_mock_timestamp() { return comp_mock_time; }

// ── Drain helper (reused from buffer_tracer tests) ──────────────

struct CompEvent {
    TimestampUs timestamp;
    std::string name;
    EventType type;
    int64_t value;
    FlowId flow_id;
};

static void comp_collect(void* ctx, const DrainEvent& event) {
    auto* vec = static_cast<std::vector<CompEvent>*>(ctx);
    vec->push_back({event.timestamp, event.name, event.type, event.value, event.flow_id});
}

static std::vector<CompEvent> drain_buf(const BufferTracer& tracer) {
    std::vector<CompEvent> events;
    tracer.drain(comp_collect, &events);
    return events;
}

// ── Tests ───────────────────────────────────────────────────────

TEST_CASE("CompositeTracer: delegates scope to all children") {
    uint8_t buf1[256], buf2[256];
    comp_mock_time = 1000;
    BufferTracer child1(buf1, sizeof(buf1), comp_mock_timestamp);
    BufferTracer child2(buf2, sizeof(buf2), comp_mock_timestamp);
    ITracer* children[] = {&child1, &child2};
    CompositeTracer tracer(children, 2);

    {
        auto guard = tracer.scope("test");
        comp_mock_time = 2000;
    }

    auto events1 = drain_buf(child1);
    auto events2 = drain_buf(child2);
    REQUIRE(events1.size() == 2);
    REQUIRE(events2.size() == 2);
    CHECK(events1[0].type == EventType::scope_enter);
    CHECK(events1[1].type == EventType::scope_exit);
    CHECK(events2[0].type == EventType::scope_enter);
    CHECK(events2[1].type == EventType::scope_exit);
}

TEST_CASE("CompositeTracer: delegates counter to all children") {
    uint8_t buf1[256], buf2[256];
    comp_mock_time = 0;
    BufferTracer child1(buf1, sizeof(buf1), comp_mock_timestamp);
    BufferTracer child2(buf2, sizeof(buf2), comp_mock_timestamp);
    ITracer* children[] = {&child1, &child2};
    CompositeTracer tracer(children, 2);

    tracer.counter("heap", 4096);

    auto events1 = drain_buf(child1);
    auto events2 = drain_buf(child2);
    REQUIRE(events1.size() == 1);
    REQUIRE(events2.size() == 1);
    CHECK(events1[0].value == 4096);
    CHECK(events2[0].value == 4096);
}

TEST_CASE("CompositeTracer: delegates flow events to all children") {
    uint8_t buf1[256], buf2[256];
    comp_mock_time = 0;
    BufferTracer child1(buf1, sizeof(buf1), comp_mock_timestamp);
    BufferTracer child2(buf2, sizeof(buf2), comp_mock_timestamp);
    ITracer* children[] = {&child1, &child2};
    CompositeTracer tracer(children, 2);

    tracer.flow_start("req", 7);
    tracer.flow_step("req", 7);
    tracer.flow_end("req", 7);

    auto events1 = drain_buf(child1);
    REQUIRE(events1.size() == 3);
    CHECK(events1[0].type == EventType::flow_start);
    CHECK(events1[1].type == EventType::flow_step);
    CHECK(events1[2].type == EventType::flow_end);
}

TEST_CASE("CompositeTracer: nested scopes work correctly") {
    uint8_t buf[512];
    comp_mock_time = 100;
    BufferTracer child(buf, sizeof(buf), comp_mock_timestamp);
    ITracer* children[] = {&child};
    CompositeTracer tracer(children, 1);

    {
        auto outer = tracer.scope("outer");
        comp_mock_time = 200;
        {
            auto inner = tracer.scope("inner");
            comp_mock_time = 300;
        }
        comp_mock_time = 400;
    }

    auto events = drain_buf(child);
    REQUIRE(events.size() == 4);
    CHECK(events[0].name == "outer");
    CHECK(events[0].type == EventType::scope_enter);
    CHECK(events[1].name == "inner");
    CHECK(events[1].type == EventType::scope_enter);
    CHECK(events[2].name == "inner");
    CHECK(events[2].type == EventType::scope_exit);
    CHECK(events[3].name == "outer");
    CHECK(events[3].type == EventType::scope_exit);
}

TEST_CASE("CompositeTracer: zero children is valid") {
    CompositeTracer tracer(nullptr, 0);

    // Should not crash
    {
        auto guard = tracer.scope("test");
    }
    tracer.counter("x", 1);
    tracer.flow_start("f", 1);
}

TEST_CASE("CompositeTracer: NullTracer child is valid") {
    auto& null = NullTracer::instance();
    ITracer* children[] = {&null};
    CompositeTracer tracer(children, 1);

    // NullTracer's scope returns no-op guard — should not crash
    {
        auto guard = tracer.scope("test");
    }
}

} // namespace et
