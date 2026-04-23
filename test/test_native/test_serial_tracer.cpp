#include <doctest/doctest.h>
#include <embedded_trace/native_print.h>
#include <embedded_trace/null_tracer.h>
#include <embedded_trace_esp32/serial_tracer.h>
#include <string>

namespace et {

// ── StringPrint: captures Print output into a std::string ────────

class StringPrint : public Print {
    std::string buffer_;
public:
    size_t write(uint8_t c) override {
        buffer_ += static_cast<char>(c);
        return 1;
    }
    size_t write(const uint8_t* data, size_t size) override {
        buffer_.append(reinterpret_cast<const char*>(data), size);
        return size;
    }
    const std::string& str() const { return buffer_; }
    void clear() { buffer_.clear(); }
    bool contains(const char* s) const { return buffer_.find(s) != std::string::npos; }
};

// ── Mock timestamp ───────────────────────────────────────────────

static TimestampUs mock_time = 0;
static TimestampUs mock_timestamp() { return mock_time; }

// ── SerialTracer: Chrome JSON output ─────────────────────────────

TEST_CASE("SerialTracer: scope emits B and E JSON events") {
    StringPrint out;
    mock_time = 1000;
    SerialTracer tracer(out, mock_timestamp);

    {
        mock_time = 1000;
        auto guard = tracer.scope("gps_fix");
        CHECK(out.contains("\"ph\":\"B\""));
        CHECK(out.contains("\"name\":\"gps_fix\""));
        CHECK(out.contains("\"ts\":1000"));
        out.clear();
        mock_time = 5000;
    }
    // guard destroyed — should emit E event
    CHECK(out.contains("\"ph\":\"E\""));
    CHECK(out.contains("\"name\":\"gps_fix\""));
    CHECK(out.contains("\"ts\":5000"));
}

TEST_CASE("SerialTracer: counter emits C JSON event") {
    StringPrint out;
    mock_time = 2000;
    SerialTracer tracer(out, mock_timestamp);

    tracer.counter("heap_free", 102400);
    CHECK(out.contains("\"ph\":\"C\""));
    CHECK(out.contains("\"name\":\"heap_free\""));
    CHECK(out.contains("\"value\":102400"));
}

TEST_CASE("SerialTracer: nested scopes produce correct order") {
    StringPrint out;
    mock_time = 0;
    SerialTracer tracer(out, mock_timestamp);

    {
        mock_time = 100;
        auto outer = tracer.scope("outer");
        {
            mock_time = 200;
            auto inner = tracer.scope("inner");
            mock_time = 300;
        }
        // inner destroyed at ts=300
        mock_time = 400;
    }
    // outer destroyed at ts=400

    // Should have: B(outer,100), B(inner,200), E(inner,300), E(outer,400)
    const auto& s = out.str();
    auto pos_b_outer = s.find("\"ph\":\"B\"");
    auto pos_b_inner = s.find("\"ph\":\"B\"", pos_b_outer + 1);
    auto pos_e_inner = s.find("\"ph\":\"E\"");
    auto pos_e_outer = s.find("\"ph\":\"E\"", pos_e_inner + 1);

    CHECK(pos_b_outer < pos_b_inner);
    CHECK(pos_b_inner < pos_e_inner);
    CHECK(pos_e_inner < pos_e_outer);
}

TEST_CASE("SerialTracer: no T= markers in output") {
    StringPrint out;
    mock_time = 1000;
    SerialTracer tracer(out, mock_timestamp);

    {
        auto guard = tracer.scope("test");
        mock_time = 2000;
    }
    CHECK_FALSE(out.contains("T="));
    CHECK(out.contains("\"ph\":\"B\""));
    CHECK(out.contains("\"ph\":\"E\""));
}

TEST_CASE("SerialTracer: pid is configurable") {
    StringPrint out;
    mock_time = 0;
    SerialTracer tracer(out, mock_timestamp, 42);

    auto guard = tracer.scope("test");
    CHECK(out.contains("\"pid\":42"));
}

// ── SerialTracer: thread ID ─────────────────────────────────────────

static ThreadId mock_tid_value = 0;
static ThreadId mock_tid() { return mock_tid_value; }

TEST_CASE("SerialTracer: default tid is 1") {
    StringPrint out;
    mock_time = 0;
    SerialTracer tracer(out, mock_timestamp);

    auto guard = tracer.scope("test");
    CHECK(out.contains("\"tid\":1"));
}

TEST_CASE("SerialTracer: tid from custom function") {
    StringPrint out;
    mock_time = 0;
    mock_tid_value = 7;
    SerialTracer tracer(out, mock_timestamp, 1, mock_tid);

    auto guard = tracer.scope("test");
    CHECK(out.contains("\"tid\":7"));
}

TEST_CASE("SerialTracer: tid in counter events") {
    StringPrint out;
    mock_time = 0;
    mock_tid_value = 3;
    SerialTracer tracer(out, mock_timestamp, 1, mock_tid);

    tracer.counter("heap", 1024);
    CHECK(out.contains("\"tid\":3"));
}

// ── SerialTracer: flow events ───────────────────────────────────────

TEST_CASE("SerialTracer: flow_start emits ph:s with id") {
    StringPrint out;
    mock_time = 1000;
    mock_tid_value = 2;
    SerialTracer tracer(out, mock_timestamp, 1, mock_tid);

    tracer.flow_start("notecard_req", 42);
    CHECK(out.contains("\"ph\":\"s\""));
    CHECK(out.contains("\"name\":\"notecard_req\""));
    CHECK(out.contains("\"id\":42"));
    CHECK(out.contains("\"tid\":2"));
    CHECK(out.contains("\"ts\":1000"));
}

TEST_CASE("SerialTracer: flow_step emits ph:t") {
    StringPrint out;
    mock_time = 2000;
    mock_tid_value = 1;
    SerialTracer tracer(out, mock_timestamp, 1, mock_tid);

    tracer.flow_step("notecard_req", 42);
    CHECK(out.contains("\"ph\":\"t\""));
    CHECK(out.contains("\"id\":42"));
    CHECK(out.contains("\"tid\":1"));
}

TEST_CASE("SerialTracer: flow_end emits ph:f") {
    StringPrint out;
    mock_time = 3000;
    mock_tid_value = 1;
    SerialTracer tracer(out, mock_timestamp, 1, mock_tid);

    tracer.flow_end("notecard_req", 42);
    CHECK(out.contains("\"ph\":\"f\""));
    CHECK(out.contains("\"id\":42"));
}

// ── SerialTracer: metadata events (ph:M) ────────────────────────────

TEST_CASE("SerialTracer: set_process_name emits ph:M process_name") {
    StringPrint out;
    SerialTracer tracer(out, mock_timestamp, 7);

    tracer.set_process_name("simple_publish");

    CHECK(out.contains("\"ph\":\"M\""));
    CHECK(out.contains("\"name\":\"process_name\""));
    CHECK(out.contains("\"pid\":7"));
    CHECK(out.contains("\"args\":{\"name\":\"simple_publish\"}"));
    // Metadata events have no timestamp — must NOT include "ts"
    CHECK_FALSE(out.contains("\"ts\":"));
}

TEST_CASE("SerialTracer: set_thread_name emits ph:M thread_name with tid") {
    StringPrint out;
    SerialTracer tracer(out, mock_timestamp, 1);

    tracer.set_thread_name(42, "loopTask");

    CHECK(out.contains("\"ph\":\"M\""));
    CHECK(out.contains("\"name\":\"thread_name\""));
    CHECK(out.contains("\"pid\":1"));
    CHECK(out.contains("\"tid\":42"));
    CHECK(out.contains("\"args\":{\"name\":\"loopTask\"}"));
    CHECK_FALSE(out.contains("\"ts\":"));
}

TEST_CASE("SerialTracer: set_thread_name emits one event per call") {
    StringPrint out;
    SerialTracer tracer(out, mock_timestamp, 1);

    tracer.set_thread_name(1, "loopTask");
    tracer.set_thread_name(2, "NotecardIO");

    const auto& s = out.str();
    auto first = s.find("loopTask");
    auto second = s.find("NotecardIO");
    CHECK(first != std::string::npos);
    CHECK(second != std::string::npos);
    CHECK(first < second);
}

TEST_CASE("ITracer: set_process_name and set_thread_name default to no-op") {
    // NullTracer inherits default no-op metadata implementations.
    auto& tracer = NullTracer::instance();
    tracer.set_process_name("anything");      // must not crash
    tracer.set_thread_name(123, "anything");  // must not crash
}

} // namespace et
