/**
 * @file extract_from.hxx
 *
 * @brief Defines functions for extracting values from strings.
 */

#ifndef CLUEAPI_SHARED_NON_COPY_EXTRACT_FROM_HXX
#define CLUEAPI_SHARED_NON_COPY_EXTRACT_FROM_HXX

namespace clueapi::shared::non_copy {
    /**
     * @brief Extracts a value as a string view from a semicolon-separated string.
     *
     * @details This function parses a string (e.g., "Content-Type: application/json;
     * charset=utf-8") into key-value pairs, trims whitespace, and returns the value associated with
     * a given key. It performs a case-insensitive comparison for the key and handles quoted values.
     *
     * @param content_type The input string view to parse.
     * @param key The key to search for.
     *
     * @return An `std::optional` containing a string view of the value if the key is found,
     * otherwise `std::nullopt`.
     */
    CLUEAPI_INLINE std::optional<std::string_view> extract_sv(
        std::string_view content_type, std::string_view key) noexcept {
        while (!content_type.empty()) {
            auto sep = content_type.find(';');

            auto part =
                (sep == std::string_view::npos) ? content_type : content_type.substr(0, sep);

            if (sep != std::string_view::npos) {
                content_type.remove_prefix(sep + 1);
            } else
                content_type = {};

            part = trim(part);

            if (part.empty())
                continue;

            auto eq = part.find('=');

            if (eq == std::string_view::npos)
                continue;

            auto k = rtrim(part.substr(0, eq));

            auto v = ltrim(part.substr(eq + 1));

            if (iequals_ascii(k, key)) {
                v = trim(v);
                v = unquote(v);

                return v;
            }
        }

        return std::nullopt;
    }

    /**
     * @brief Extracts a value as a `std::string` from a semicolon-separated string.
     *
     * @details This function calls `extract_sv` and if a value is found, it copies
     * the result into a new `std::string`.
     *
     * @param content_type The input string view to parse.
     * @param key The key to search for.
     *
     * @return The value as a `std::string` if the key is found, otherwise an empty string.
     */
    CLUEAPI_INLINE std::string extract_str(
        std::string_view content_type, std::string_view key) noexcept {
        if (auto sv = extract_sv(content_type, key))
            return std::string{sv.value()};

        return {};
    }
} // namespace clueapi::shared::non_copy

#endif // CLUEAPI_SHARED_NON_COPY_EXTRACT_FROM_HXX