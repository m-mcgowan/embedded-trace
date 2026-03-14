#pragma once

/**
 * @brief Minimal Print base class for non-Arduino builds.
 *
 * Matches the subset of Arduino's Print interface used by embedded-tracer.
 * On Arduino, <Print.h> provides the real implementation.
 *
 * All methods route through write() so subclasses only need to override
 * write() to capture all output (matching Arduino's Print behavior).
 */

#ifndef ARDUINO

#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <cstdio>
#include <cstring>

class Print {
public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t* buf, size_t size) {
        size_t n = 0;
        while (size--) n += write(*buf++);
        return n;
    }

    virtual void print(const char* s) {
        if (s) write(reinterpret_cast<const uint8_t*>(s), strlen(s));
    }
    virtual void print(int v) {
        char buf[16];
        int n = snprintf(buf, sizeof(buf), "%d", v);
        if (n > 0) write(reinterpret_cast<const uint8_t*>(buf), n);
    }
    virtual void println() { write('\n'); }
    virtual void println(const char* s) { print(s); write('\n'); }
    virtual int printf(const char* fmt, ...) {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        if (n > 0) write(reinterpret_cast<const uint8_t*>(buf), n);
        return n;
    }
};

#endif // ARDUINO
