// embedded-trace: basic scope tracing on the host.
//
// Build & run from this directory:
//   pio run -e basic_scopes -t exec
//
// Demonstrates: scope nesting, counters, metadata events, RAII scope
// lifetime via TRACE_SCOPE. Output is Chrome Trace Event JSON, one
// event per line, to stdout. Pipe into a file and load in Perfetto:
//   pio run -e basic_scopes -t exec > trace.json
//   # then wrap as {"traceEvents":[...]} and open in https://ui.perfetto.dev

#include <embedded_trace/native_print.h>
#include <embedded_trace/trace_macros.h>
#include <embedded_trace_esp32/serial_tracer.h>

#include <chrono>
#include <cstdio>
#include <thread>

// Print subclass that writes to stdout — SerialTracer's output sink.
class StdoutPrint : public Print {
public:
    size_t write(uint8_t c) override {
        std::fputc(c, stdout);
        return 1;
    }
    size_t write(const uint8_t* buf, size_t size) override {
        std::fwrite(buf, 1, size, stdout);
        return size;
    }
};

// Microsecond timestamp from monotonic clock. SerialTracer truncates
// to uint32_t (~71 minutes before wrap — fine for an example).
static et::TimestampUs micros() {
    static const auto t0 = std::chrono::steady_clock::now();
    auto delta = std::chrono::steady_clock::now() - t0;
    return static_cast<et::TimestampUs>(
        std::chrono::duration_cast<std::chrono::microseconds>(delta).count());
}

static void busy_work(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

static void process_batch(et::ITracer& tracer, int batch_id) {
    // Explicit (cat, name) form — renders as "cat":"batch","name":"process".
    auto outer = tracer.scope("batch", "process");

    {
        // Dotted single-arg form — tracer auto-splits on first dot.
        // Renders identically: "cat":"batch","name":"validate".
        auto g = tracer.scope("batch.validate");
        busy_work(2);
    }
    {
        auto g = tracer.scope("batch.transform");
        busy_work(5);
        TRACE_COUNTER(tracer, "items_processed", batch_id * 10);
    }
}

int main() {
    StdoutPrint out;
    et::SerialTracer tracer(out, micros);

    // One-shot metadata at trace start — Perfetto shows readable lane names.
    tracer.set_process_name("basic_scopes_example");
    tracer.set_thread_name(1, "main");

    {
        TRACE_SCOPE(tracer, "main_loop");
        for (int i = 1; i <= 3; ++i) {
            process_batch(tracer, i);
            busy_work(1);
        }
    }

    return 0;
}
