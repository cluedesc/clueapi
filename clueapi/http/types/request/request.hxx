/**
 * @file request.hxx
 *
 * @brief Defines the structure representing an incoming HTTP request.
 */

#ifndef CLUEAPI_HTTP_TYPES_REQUEST_HXX
#define CLUEAPI_HTTP_TYPES_REQUEST_HXX

namespace clueapi::http::types {
    /**
     * @struct request_t
     *
     * @brief Represents a single HTTP request received by the server.
     *
     * @details This class provides a high-level abstraction over a raw HTTP request. It offers
     * convenient access to properties like the method, URI, headers, body, and parsed cookies.
     * It also handles lazy parsing of cookies to optimize performance.
     */
    struct request_t {
        CLUEAPI_INLINE request_t() noexcept = default;

        CLUEAPI_INLINE ~request_t() noexcept = default;

        // Copy constructor
        CLUEAPI_INLINE request_t(const request_t& other) noexcept
            : m_method(other.m_method),
              m_uri(other.m_uri),
              m_body(other.m_body),
              m_headers(other.m_headers),
              m_parse_path(other.m_parse_path),
              m_cookies_parsed(other.m_cookies_parsed) {
            if (other.m_cookies_parsed) {
                try {
                    m_cookies_buffer = other.m_cookies_buffer;

                    m_cookies.reserve(other.m_cookies.size());

                    for (const auto& [key_view, value_view] : other.m_cookies) {
                        const auto key_offset = key_view.data() - other.m_cookies_buffer.data();
                        const auto value_offset = value_view.data() - other.m_cookies_buffer.data();

                        std::string_view new_key_view(
                            m_cookies_buffer.data() + key_offset, key_view.length());

                        std::string_view new_value_view(
                            m_cookies_buffer.data() + value_offset, value_view.length());

                        m_cookies.emplace(new_key_view, new_value_view);
                    }
                } catch (...) {
                    // ...
                }
            }
        }

        // Copy assignment operator
        CLUEAPI_INLINE request_t& operator=(const request_t& other) noexcept {
            if (this == &other)
                return *this;

            try {
                m_method = other.m_method;
                m_uri = other.m_uri;
                m_body = other.m_body;
                m_headers = other.m_headers;
                m_parse_path = other.m_parse_path;
                m_cookies_parsed = other.m_cookies_parsed;

                m_cookies.clear();

                m_cookies_buffer.clear();

                if (other.m_cookies_parsed) {
                    m_cookies_buffer = other.m_cookies_buffer;

                    m_cookies.reserve(other.m_cookies.size());

                    for (const auto& [key_view, value_view] : other.m_cookies) {
                        const auto key_offset = key_view.data() - other.m_cookies_buffer.data();
                        const auto value_offset = value_view.data() - other.m_cookies_buffer.data();

                        std::string_view new_key_view(
                            m_cookies_buffer.data() + key_offset, key_view.length());

                        std::string_view new_value_view(
                            m_cookies_buffer.data() + value_offset, value_view.length());

                        m_cookies.emplace(new_key_view, new_value_view);
                    }
                }
            } catch (...) {
                // ...
            }

            return *this;
        }

        // Move constructor
        CLUEAPI_INLINE request_t(request_t&&) noexcept = default;

        // Move assignment operator
        CLUEAPI_INLINE request_t& operator=(request_t&&) noexcept = default;

       public:
        /**
         * @brief Retrieves the value of a specific cookie from the request.
         *
         * @param name The name of the cookie to retrieve.
         *
         * @return An `std::optional<std::string_view>` containing the cookie's value if found,
         * otherwise `std::nullopt`.
         *
         * @details Cookies are parsed on the first call to this function or `cookies()`.
         */
        CLUEAPI_INLINE std::optional<std::string_view> cookie(
            const std::string_view& name) const noexcept {
            if (!m_cookies_parsed)
                parse_cookies();

            auto cookie = m_cookies.find(name);

            if (cookie == m_cookies.end())
                return std::nullopt;

            return cookie->second;
        }

        /**
         * @brief Retrieves the value of a specific header from the request.
         *
         * @param name The name of the header to retrieve (case-insensitive).
         *
         * @return An `std::optional<std::string_view>` containing the header's value if found,
         * otherwise`std::nullopt`.
         */
        CLUEAPI_INLINE std::optional<std::string_view> header(
            const std::string_view& name) const noexcept {
            auto value = m_headers.find(name);

            if (value == m_headers.end())
                return std::nullopt;

            return value->second;
        }

        /**
         * @brief Checks if the client requested a persistent connection.
         *
         * @return `true` if the `Connection` header is "keep-alive" or absent, `false` otherwise.
         */
        CLUEAPI_INLINE bool keep_alive() const noexcept {
            auto keep_alive = header("connection");

            if (!keep_alive.has_value())
                return true;

            return boost::iequals(keep_alive.value(), "keep-alive");
        }

        /**
         * @brief Resets the request object to a default state for reuse.
         */
        CLUEAPI_INLINE void reset() noexcept {
            m_method = method_t::unknown;

            m_uri.clear();

            m_body.clear();

            m_headers.clear();

            m_parse_path.clear();

            m_cookies_parsed = false;

            m_cookies.clear();

            m_cookies_buffer.clear();
        }

       public:
        /**
         * @brief Gets a mutable reference to the HTTP method.
         *
         * @return A reference to the method enum.
         */
        CLUEAPI_INLINE auto& method() noexcept {
            return m_method;
        }

        /**
         * @brief Gets a const reference to the HTTP method.
         *
         * @return A const reference to the method enum.
         */
        CLUEAPI_INLINE const auto& method() const noexcept {
            return m_method;
        }

        /**
         * @brief Gets a mutable reference to the request URI.
         *
         * @return A reference to the URI string.
         */
        CLUEAPI_INLINE auto& uri() noexcept {
            return m_uri;
        }

        /**
         * @brief Gets a const reference to the request URI.
         *
         * @return A const reference to the URI string.
         */
        CLUEAPI_INLINE const auto& uri() const noexcept {
            return m_uri;
        }

        /**
         * @brief Gets a mutable reference to the request body.
         *
         * @return A reference to the body string.
         */
        CLUEAPI_INLINE auto& body() noexcept {
            return m_body;
        }

        /**
         * @brief Gets a const reference to the request body.
         *
         * @return A const reference to the body string.
         */
        CLUEAPI_INLINE const auto& body() const noexcept {
            return m_body;
        }

        /**
         * @brief Gets a mutable reference to the request headers.
         *
         * @return A reference to the headers map.
         */
        CLUEAPI_INLINE auto& headers() noexcept {
            return m_headers;
        }

        /**
         * @brief Gets a const reference to the request headers.
         *
         * @return A const reference to the headers map.
         */
        CLUEAPI_INLINE const auto& headers() const noexcept {
            return m_headers;
        }

        /**
         * @brief Gets a mutable reference to the path where a large multipart body was saved.
         *
         * @return A reference to the `boost::filesystem::path`.
         */
        CLUEAPI_INLINE auto& parse_path() noexcept {
            return m_parse_path;
        }

        /**
         * @brief Gets a const reference to the path where a large multipart body was saved.
         *
         * @return A const reference to the `boost::filesystem::path`.
         */
        CLUEAPI_INLINE const auto& parse_path() const noexcept {
            return m_parse_path;
        }

        /**
         * @brief Gets a const reference to the parsed cookies.
         *
         * @return A const reference to the cookies map.
         *
         * @details Cookies are parsed on the first call to this function or `cookie()`.
         */
        CLUEAPI_INLINE const auto& cookies() const noexcept {
            if (!m_cookies_parsed)
                parse_cookies();

            return m_cookies;
        }

       private:
        /**
         * @brief Lazily parses the `Cookie` header from the request.
         *
         * @details This function is marked `mutable` to be callable from const methods.
         */
        void parse_cookies() const noexcept;

       private:
        /**
         * @brief The HTTP method of the request.
         */
        method_t::e_method m_method{};

        /**
         * @brief The URI of the request.
         */
        uri_t m_uri;

        /**
         * @brief The request body.
         */
        body_t m_body;

        /**
         * @brief The HTTP headers of the request.
         */
        headers_t m_headers;

        /**
         * @brief The path where a large multipart body was saved.
         */
        boost::filesystem::path m_parse_path;

       private:
        /**
         * @brief The parsed cookies from the request.
         *
         * @internal
         */
        mutable req_cookies_t m_cookies;

        /**
         * @brief Whether the cookies have been parsed.
         *
         * @internal
         */
        mutable bool m_cookies_parsed{};

        /**
         * @brief The raw cookies string.
         *
         * @internal
         */
        mutable std::string m_cookies_buffer;
    };
} // namespace clueapi::http::types

#endif // CLUEAPI_HTTP_TYPES_REQUEST_HXX