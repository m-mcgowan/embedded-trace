#include <doctest/doctest.h>
#include <embedded_trace/scope_guard.h>
#include <embedded_trace/null_tracer.h>
#include <embedded_trace/i_tracer.h>

namespace et {

// ── Test helpers ──────────────────────────────────────────────────

static int exit_call_count = 0;
static const char* last_exit_name = nullptr;
static ScopeId last_exit_id = 0;

static void test_exit_fn(void* context, const char* name, ScopeId scope_id) {
    (void)context;
    exit_call_count++;
    last_exit_name = name;
    last_exit_id = scope_id;
}

static void reset_counters() {
    exit_call_count = 0;
    last_exit_name = nullptr;
    last_exit_id = 0;
}

// ── ScopeGuard tests ─────────────────────────────────────────────

TEST_CASE("ScopeGuard: null exit_fn is no-op") {
    reset_counters();
    {
        ScopeGuard guard(nullptr, nullptr, "test", 0);
    }
    CHECK(exit_call_count == 0);
}

TEST_CASE("ScopeGuard: calls exit_fn on destruction") {
    reset_counters();
    {
        ScopeGuard guard(nullptr, test_exit_fn, "my_scope", 42);
    }
    CHECK(exit_call_count == 1);
    CHECK_EQ(last_exit_name, "my_scope");
    CHECK_EQ(last_exit_id, 42);
}

TEST_CASE("ScopeGuard: move prevents double exit") {
    reset_counters();
    {
        ScopeGuard guard1(nullptr, test_exit_fn, "moved", 7);
        ScopeGuard guard2(std::move(guard1));
        // guard1 should not fire on destruction
    }
    CHECK(exit_call_count == 1);
    CHECK_EQ(last_exit_name, "moved");
}

TEST_CASE("ScopeGuard: move assignment ends current scope") {
    reset_counters();
    {
        ScopeGuard guard1(nullptr, test_exit_fn, "first", 1);
        ScopeGuard guard2(nullptr, test_exit_fn, "second", 2);
        guard1 = std::move(guard2);
        // "first" should have been ended by the assignment
        CHECK(exit_call_count == 1);
        CHECK_EQ(last_exit_name, "first");
    }
    // "second" (now in guard1) should be ended on destruction
    CHECK(exit_call_count == 2);
    CHECK_EQ(last_exit_name, "second");
}

TEST_CASE("ScopeGuard: default constructor is no-op") {
    reset_counters();
    {
        ScopeGuard guard;
    }
    CHECK(exit_call_count == 0);
}

TEST_CASE("ScopeGuard: end() fires exit_fn and disables dtor") {
    reset_counters();
    {
        ScopeGuard guard(nullptr, test_exit_fn, "manual", 99);
        guard.end();
        CHECK(exit_call_count == 1);
        CHECK_EQ(last_exit_name, "manual");
        CHECK_EQ(last_exit_id, 99);
        // dtor must NOT fire again when guard goes out of scope
    }
    CHECK(exit_call_count == 1);
}

TEST_CASE("ScopeGuard: end() is idempotent") {
    reset_counters();
    ScopeGuard guard(nullptr, test_exit_fn, "twice", 5);
    guard.end();
    guard.end();
    guard.end();
    CHECK(exit_call_count == 1);
}

TEST_CASE("ScopeGuard: end() on default-constructed guard is no-op") {
    reset_counters();
    ScopeGuard guard;
    guard.end();
    CHECK(exit_call_count == 0);
}

TEST_CASE("ScopeGuard: end() then move-assign behaves correctly") {
    reset_counters();
    {
        ScopeGuard g1(nullptr, test_exit_fn, "a", 1);
        g1.end();
        CHECK(exit_call_count == 1);
        // g1 is now ended; move-assigning into it must not fire any exit
        ScopeGuard g2(nullptr, test_exit_fn, "b", 2);
        g1 = std::move(g2);
        CHECK(exit_call_count == 1);  // no double-close of "a"
    }
    // "b" (now in g1) ends on dtor
    CHECK(exit_call_count == 2);
    CHECK_EQ(last_exit_name, "b");
}

// ── NullTracer tests ─────────────────────────────────────────────

TEST_CASE("NullTracer: scope returns no-op guard") {
    reset_counters();
    auto& tracer = NullTracer::instance();
    {
        auto guard = tracer.scope("test");
    }
    // NullTracer passes nullptr for exit_fn — should not call anything
    CHECK(exit_call_count == 0);
}

TEST_CASE("NullTracer: counter is no-op") {
    auto& tracer = NullTracer::instance();
    tracer.counter("heap", 42);  // should not crash
}

TEST_CASE("NullTracer: singleton identity") {
    auto& a = NullTracer::instance();
    auto& b = NullTracer::instance();
    CHECK(&a == &b);
}

} // namespace et
