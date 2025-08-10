/**
 * @file cookie.hxx
 *
 * @brief Defines structures and types for handling HTTP cookies.
 */

#ifndef CLUEAPI_HTTP_TYPES_COOKIE_HXX
#define CLUEAPI_HTTP_TYPES_COOKIE_HXX

#include "detail/detail.hxx"

namespace clueapi::http::types {
    /**
     * @brief Type alias for a raw, serialized `Set-Cookie` header string.
     */
    using raw_cookie_t = std::string;

    /**
     * @struct cookie_t
     *
     * @brief Represents an HTTP cookie to be sent in a response.
     *
     * @details This struct provides a high-level interface for creating and configuring
     * an HTTP cookie with all its standard attributes. It can be serialized into a raw string
     * suitable for a `Set-Cookie` header.
     * By default, cookies are configured with the following attributes:
     * - `Path`: "/"
     * - `Max-Age`: 24 hours
     * - `SameSite`: "Lax"
     */
    struct cookie_t {
        CLUEAPI_INLINE constexpr cookie_t() = default;

        /**
         * @brief Constructs a cookie with a name and value.
         *
         * @param name The name of the cookie.
         * @param value The value of the cookie.
         */
        CLUEAPI_INLINE cookie_t(std::string_view name, std::string_view value) : m_name(name), m_value(value) {}

      public:
        /**
         * @brief Serializes the current cookie instance into a `Set-Cookie` header string.
         *
         * @return An `expected` containing the serialized string on success, or an error.
         */
        exceptions::expected_t<raw_cookie_t> serialize();

        /**
         * @brief Statically serializes a cookie instance into a `Set-Cookie` header string.
         *
         * @param cookie The cookie object to serialize.
         *
         * @return An `expected` containing the serialized string on success, or an error.
         */
        static exceptions::expected_t<raw_cookie_t> serialize(const cookie_t& cookie);

      public:
        /**
         * @brief Gets a mutable reference to the cookie's name.
         * @return A reference to the name.
         */
        CLUEAPI_INLINE auto& name() { return m_name; }

        /**
         * @brief Gets a const reference to the cookie's name.
         * @return A const reference to the name.
         */
        CLUEAPI_INLINE const auto& name() const { return m_name; }

        /**
         * @brief Gets a mutable reference to the cookie's value.
         * @return A reference to the value.
         */
        CLUEAPI_INLINE auto& value() { return m_value; }

        /**
         * @brief Gets a const reference to the cookie's value.
         * @return A const reference to the value.
         */
        CLUEAPI_INLINE const auto& value() const { return m_value; }

        /**
         * @brief Gets a mutable reference to the cookie's path attribute.
         * @return A reference to the path.
         */
        CLUEAPI_INLINE auto& path() { return m_path; }

        /**
         * @brief Gets a const reference to the cookie's path attribute.
         * @return A const reference to the path.
         */
        CLUEAPI_INLINE const auto& path() const { return m_path; }

        /**
         * @brief Gets a mutable reference to the cookie's domain attribute.
         * @return A reference to the domain.
         */
        CLUEAPI_INLINE auto& domain() { return m_domain; }

        /**
         * @brief Gets a const reference to the cookie's domain attribute.
         * @return A const reference to the domain.
         */
        CLUEAPI_INLINE const auto& domain() const { return m_domain; }

        /**
         * @brief Gets a mutable reference to the cookie's secure flag.
         * @return A reference to the secure flag.
         */
        CLUEAPI_INLINE auto& secure() { return m_secure; }

        /**
         * @brief Gets a const reference to the cookie's secure flag.
         * @return A const reference to the secure flag.
         */
        CLUEAPI_INLINE const auto& secure() const { return m_secure; }

        /**
         * @brief Gets a mutable reference to the cookie's HttpOnly flag.
         * @return A reference to the HttpOnly flag.
         */
        CLUEAPI_INLINE auto& http_only() { return m_http_only; }

        /**
         * @brief Gets a const reference to the cookie's HttpOnly flag.
         * @return A const reference to the HttpOnly flag.
         */
        CLUEAPI_INLINE const auto& http_only() const { return m_http_only; }

        /**
         * @brief Gets a mutable reference to the cookie's Max-Age attribute.
         * @return A reference to the Max-Age value.
         */
        CLUEAPI_INLINE auto& max_age() { return m_max_age; }

        /**
         * @brief Gets a const reference to the cookie's Max-Age attribute.
         * @return A const reference to the Max-Age value.
         */
        CLUEAPI_INLINE const auto& max_age() const { return m_max_age; }

        /**
         * @brief Gets a mutable reference to the cookie's SameSite attribute.
         * @return A reference to the SameSite value.
         */
        CLUEAPI_INLINE auto& same_site() { return m_same_site; }

      private:
        /**
         * @brief The name of the cookie.
         */
        std::string_view m_name{};

        /**
         * @brief The value of the cookie.
         */
        std::string_view m_value{};

        /**
         * @brief The path of the cookie.
         */
        std::string_view m_path{"/"};

        /**
         * @brief The domain of the cookie.
         */
        std::string_view m_domain{""};

        /**
         * @brief Whether the cookie is secure.
         */
        bool m_secure{false};

        /**
         * @brief Whether the cookie is HTTP-only.
         */
        bool m_http_only{false};

        /**
         * @brief The maximum age of the cookie.
         */
        std::chrono::seconds m_max_age{std::chrono::hours{24}};

        /**
         * @brief The SameSite attribute of the cookie.
         */
        std::string_view m_same_site{"Lax"};
    };

    /**
     * @brief Type alias for a map of cookies parsed from a request's `Cookie` header.
     */
    using req_cookies_t = shared::unordered_map<std::string_view, std::string_view>;

    /**
     * @brief Type alias for a vector of raw cookie strings to be sent in a response's `Set-Cookie` headers.
     */
    using resp_cookies_t = std::vector<raw_cookie_t>;
}

#endif // CLUEAPI_HTTP_TYPES_COOKIE_HXX