#include <doctest/doctest.h>
#include <embedded_trace/buffered_print.h>
#include <embedded_trace/native_print.h>

#include <string>

namespace et {

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
};

static bool g_ready = false;
static bool ready_fn() { return g_ready; }

TEST_CASE("BufferedPrint: buffers while not ready, flushes on first ready write") {
    g_ready = false;
    StringPrint sink;
    uint8_t buf[256];
    BufferedPrint bp(sink, buf, sizeof(buf), ready_fn);

    const char* a = "event-a\n";
    bp.write(reinterpret_cast<const uint8_t*>(a), 8);
    CHECK(sink.str().empty());
    CHECK(bp.buffered() == 8);
    CHECK_FALSE(bp.is_flushed());

    const char* b = "event-b\n";
    bp.write(reinterpret_cast<const uint8_t*>(b), 8);
    CHECK(sink.str().empty());
    CHECK(bp.buffered() == 16);

    g_ready = true;
    const char* c = "event-c\n";
    bp.write(reinterpret_cast<const uint8_t*>(c), 8);
    // Flush then passthrough — sink should have all three events in order.
    CHECK(sink.str() == "event-a\nevent-b\nevent-c\n");
    CHECK(bp.buffered() == 0);
    CHECK(bp.is_flushed());
}

TEST_CASE("BufferedPrint: passthrough when ready from the start") {
    g_ready = true;
    StringPrint sink;
    uint8_t buf[64];
    BufferedPrint bp(sink, buf, sizeof(buf), ready_fn);

    bp.write(reinterpret_cast<const uint8_t*>("hello\n"), 6);
    CHECK(sink.str() == "hello\n");
    CHECK(bp.is_flushed());
    CHECK_FALSE(bp.overflowed());
}

TEST_CASE("BufferedPrint: once flushed stays flushed even if predicate flips") {
    g_ready = true;
    StringPrint sink;
    uint8_t buf[64];
    BufferedPrint bp(sink, buf, sizeof(buf), ready_fn);

    bp.write(reinterpret_cast<const uint8_t*>("a"), 1);
    CHECK(bp.is_flushed());

    // Simulated disconnect after first flush — subsequent writes pass through
    // rather than re-buffering. Deep-sleep wake cycles use a short firmware
    // wait in app_setup, so this matches the real-world sequence.
    g_ready = false;
    bp.write(reinterpret_cast<const uint8_t*>("b"), 1);
    CHECK(sink.str() == "ab");
}

TEST_CASE("BufferedPrint: overflow drops newest bytes, preserves earliest") {
    g_ready = false;
    StringPrint sink;
    uint8_t buf[4];
    BufferedPrint bp(sink, buf, sizeof(buf), ready_fn);

    bp.write(reinterpret_cast<const uint8_t*>("abcdef"), 6);
    CHECK(bp.overflowed());
    CHECK(bp.buffered() == 4);

    g_ready = true;
    bp.write(reinterpret_cast<const uint8_t*>("X"), 1);
    // Dropped "ef"; kept "abcd" (earliest bytes), then pass-through "X".
    CHECK(sink.str() == "abcdX");
}

TEST_CASE("BufferedPrint: single-byte write buffers like multi-byte write") {
    g_ready = false;
    StringPrint sink;
    uint8_t buf[16];
    BufferedPrint bp(sink, buf, sizeof(buf), ready_fn);

    bp.write(static_cast<uint8_t>('x'));
    bp.write(static_cast<uint8_t>('y'));
    CHECK(bp.buffered() == 2);
    CHECK(sink.str().empty());

    g_ready = true;
    bp.write(static_cast<uint8_t>('z'));
    CHECK(sink.str() == "xyz");
}

} // namespace et
