#include <doctest/doctest.h>
#include <embedded_tracer/native_print.h>
#include <embedded_tracer_esp32/serial_tracer.h>
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

// ── SerialTracer: PPK2 event markers ─────────────────────────────

TEST_CASE("SerialTracer: ppk2_markers emits T= lines") {
    StringPrint out;
    mock_time = 1600;
    SerialTracer tracer(out, mock_timestamp, true);

    {
        auto guard = tracer.scope("gps_fix");
        CHECK(out.contains("T=0.001600 GPS_FIX_STARTED"));
        CHECK(out.contains("\"ph\":\"B\""));
        out.clear();
        mock_time = 4200;
    }
    CHECK(out.contains("T=0.004200 GPS_FIX_STOPPED"));
    CHECK(out.contains("\"ph\":\"E\""));
}

TEST_CASE("SerialTracer: ppk2_markers=false omits T= lines") {
    StringPrint out;
    mock_time = 1000;
    SerialTracer tracer(out, mock_timestamp, false);

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
    SerialTracer tracer(out, mock_timestamp, false, 42);

    auto guard = tracer.scope("test");
    CHECK(out.contains("\"pid\":42"));
}

} // namespace et
