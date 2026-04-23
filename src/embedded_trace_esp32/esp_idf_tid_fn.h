#pragma once

/**
 * @file esp_idf_tid_fn.h
 * @brief FreeRTOS task-handle-based ThreadIdFn for SerialTracer.
 *
 * Pass to SerialTracer to get one swim lane per FreeRTOS task in
 * Perfetto, instead of the constant-1 default. SerialTracer also
 * uses this automatically when ESP_PLATFORM is defined and no
 * tid_fn is supplied.
 *
 * Example:
 * @code
 * SerialTracer tracer(Serial, micros, 1, et::esp_idf_tid_fn);
 * @endcode
 *
 * Header is empty on non-ESP_PLATFORM builds so it is safe to include
 * unconditionally.
 */

#ifdef ESP_PLATFORM

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdint>

#include "../embedded_trace/types.h"

namespace et {

/// FreeRTOS task handle as ThreadId. One lane per task in Perfetto.
inline ThreadId esp_idf_tid_fn() {
    return static_cast<ThreadId>(reinterpret_cast<uintptr_t>(xTaskGetCurrentTaskHandle()));
}

} // namespace et

#endif // ESP_PLATFORM
