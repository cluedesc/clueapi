/**
 * @file mime.hxx
 *
 * @brief Defines type aliases related to MIME types.
 */

#ifndef CLUEAPI_HTTP_TYPES_MIME_HXX
#define CLUEAPI_HTTP_TYPES_MIME_HXX

namespace clueapi::http::types {
    /**
     * @brief Type alias for a string view representing a MIME type (e.g., "text/html").
     */
    using mime_type_t = std::string_view;

    /**
     * @brief Type alias for a map used to store file extensions and their corresponding MIME types.
     *
     * @details This uses a fast, dense hash map for efficient lookups.
     * The key is the file extension (e.g., ".html"), and the value is the `mime_type_t`.
     */
    using mime_map_t = shared::unordered_map<
        std::string_view,
        mime_type_t,
        http::detail::sv_hash_t,
        http::detail::sv_eq_t>;
} // namespace clueapi::http::types

#endif // CLUEAPI_HTTP_TYPES_MIME_HXX