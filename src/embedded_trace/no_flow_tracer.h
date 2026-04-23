#pragma once

#include "i_tracer.h"

namespace et {

/**
 * @brief ITracer mixin that no-ops the flow_* methods.
 *
 * Inherit from this instead of ITracer directly if your tracer doesn't
 * represent causal flow (e.g. pin toggles, simple text logs). You still
 * must implement scope(), counter(), set_process_name(), and
 * set_thread_name() — or inherit NoMetadataTracer too / alongside.
 */
class NoFlowTracer : public ITracer {
public:
    void flow_start(const char* /*name*/, FlowId /*id*/) override {}
    void flow_step(const char* /*name*/, FlowId /*id*/) override {}
    void flow_end(const char* /*name*/, FlowId /*id*/) override {}
};

} // namespace et
