#pragma once

#include "i_tracer.h"

namespace et {

/**
 * @brief ITracer mixin that no-ops the metadata methods.
 *
 * Inherit from this instead of ITracer directly if your tracer has no
 * host-side use for process/thread names (e.g. BufferTracer — binary
 * drain has no JSON metadata to carry). You still must implement
 * scope(), counter(), and flow_* — or inherit NoFlowTracer too /
 * alongside.
 */
class NoMetadataTracer : public ITracer {
public:
    void set_process_name(const char* /*name*/) override {}
    void set_thread_name(ThreadId /*tid*/, const char* /*name*/) override {}
};

} // namespace et
