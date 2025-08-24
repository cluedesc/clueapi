/**
 * @file basic.hxx
 *
 * @brief Defines fundamental type aliases used throughout the HTTP module.
 */

#ifndef CLUEAPI_HTTP_TYPES_BASIC_HXX
#define CLUEAPI_HTTP_TYPES_BASIC_HXX

/**
 * @namespace clueapi::http::types
 *
 * @brief The main namespace for the clueapi HTTP types.
 */
namespace clueapi::http::types {
    /**
     * @brief Type alias for a URI string.
     */
    using uri_t = std::string;

    /**
     * @brief Type alias for an HTTP request or response body.
     */
    using body_t = std::string;

    /**
     * @brief Type alias for a string containing HTML content.
     */
    using html_t = std::string;

    /**
     * @brief Type alias for a URL path string.
     */
    using path_t = std::string;

    /**
     * @brief Type alias for the JSON traits, providing serialization and deserialization
     * capabilities.
     */
    using json_t = shared::json_traits_t;

    /**
     * @brief Type alias for a map of HTTP headers, with case-insensitive keys.
     */
    using headers_t = std::map<std::string, std::string, http::detail::ci_less_t>;

    /**
     * @brief Type alias for a map of URL parameters, with case-insensitive keys.
     */
    using params_t = std::map<std::string, std::string, http::detail::ci_less_t>;
} // namespace clueapi::http::types

#endif // CLUEAPI_HTTP_TYPES_BASIC_HXX