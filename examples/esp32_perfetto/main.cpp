// embedded-trace: ESP32-S3 → Serial → Perfetto.
//
// Build & flash:
//   pio run -e esp32_perfetto -t upload
//   pio device monitor -b 115200 > trace.jsonl
//
// Then wrap as {"traceEvents":[...]} and load in https://ui.perfetto.dev.
//
// Demonstrates: scope nesting on multiple FreeRTOS tasks (one Perfetto
// lane per task via the default esp_idf_tid_fn), counters, metadata
// events for human-readable lane names.

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <embedded_trace/trace_macros.h>
#include <embedded_trace_esp32/serial_tracer.h>
#include <embedded_trace_esp32/esp_idf_tid_fn.h>

static et::TimestampUs micros_fn() { return micros(); }

// Single tracer instance shared across tasks. SerialTracer's default
// tid_fn on ESP_PLATFORM is esp_idf_tid_fn — one Perfetto lane per
// FreeRTOS task, no manual wiring needed.
static et::SerialTracer tracer(Serial, micros_fn);

static void sensor_task(void*) {
    // Name this task's lane in Perfetto.
    tracer.set_thread_name(et::esp_idf_tid_fn(), "sensor");

    while (true) {
        {
            TRACE_SCOPE(tracer, "sensor.sample");
            vTaskDelay(pdMS_TO_TICKS(15));   // simulate ADC read
        }
        TRACE_COUNTER(tracer, "sensor.value", random(0, 4096));
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);   // give USB-CDC a chance to enumerate before the first event

    tracer.set_process_name("esp32_perfetto_example");
    tracer.set_thread_name(et::esp_idf_tid_fn(), "loopTask");

    xTaskCreate(sensor_task, "sensor", 4096, nullptr, 5, nullptr);
}

void loop() {
    TRACE_SCOPE(tracer, "loop.cycle");

    {
        TRACE_SCOPE(tracer, "loop.work");
        delay(20);
    }
    TRACE_COUNTER(tracer, "heap.free", ESP.getFreeHeap());

    delay(100);
}
