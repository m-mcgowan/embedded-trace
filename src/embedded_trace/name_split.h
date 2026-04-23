#pragma once

#include <cstddef>
#include <cstring>

namespace et {

/**
 * @brief Result of splitting a scope identifier into (category, name).
 *
 * @p cat_len is the length of the category prefix in the original
 * buffer; it is not a C string (may not be null-terminated at that
 * offset). Consumers should use `%.*s` / `snprintf`-style bounded
 * copies when rendering.
 *
 * When no category is present, @p cat is `nullptr` and @p cat_len
 * is 0.
 */
struct SplitName {
    const char* cat;
    size_t cat_len;
    const char* name;
};

/**
 * @brief Split a scope identifier into (category, name) on the first dot.
 *
 * Resolution order:
 *   1. If @p explicit_cat is non-null, use it verbatim; @p name_or_full
 *      is the name (no splitting, even if it contains dots).
 *   2. Else, find the first '.' in @p name_or_full and split there.
 *   3. Else (no dot), no category — @p cat is null, @p name is the
 *      whole input.
 *
 * Consumers: SerialTracer at JSON emit time, BufferTracer at drain time.
 * Both tracers must agree on the split rule — this function is the
 * single source of truth.
 */
inline SplitName split_scope_name(const char* name_or_full, const char* explicit_cat) {
    if (explicit_cat) {
        return SplitName{explicit_cat, std::strlen(explicit_cat), name_or_full};
    }
    if (name_or_full) {
        if (const char* dot = std::strchr(name_or_full, '.')) {
            return SplitName{name_or_full, static_cast<size_t>(dot - name_or_full), dot + 1};
        }
    }
    return SplitName{nullptr, 0, name_or_full};
}

} // namespace et
