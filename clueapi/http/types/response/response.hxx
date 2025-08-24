/**
 * @file response.hxx
 *
 * @brief Defines the structures for creating and managing HTTP responses.
 */

#ifndef CLUEAPI_HTTP_TYPES_RESPONSE_HXX
#define CLUEAPI_HTTP_TYPES_RESPONSE_HXX

namespace clueapi::http::types {
    /**
     * @struct base_response_t
     *
     * @brief The base class for all HTTP response types.
     *
     * @details This class provides the core functionality for an HTTP response, including
     * managing the body, headers, cookies, and status code. It also supports streaming
     * responses via a function callback. It is designed to be inherited by more
     * specific response types (e.g., `html_response_t`, `json_response_t`).
     */
    struct base_response_t {
        /**
         * @brief A function object for streaming a response body using a `chunk_writer_t`.
         */
        using stream_fn_t =
            std::function<exceptions::expected_awaitable_t<void>(chunks::chunk_writer_t&)>;

       public:
        CLUEAPI_INLINE constexpr base_response_t() noexcept = default;

        CLUEAPI_INLINE virtual ~base_response_t() noexcept = default;

       public:
        /**
         * @brief Constructs a basic response.
         *
         * @param body The response body.
         * @param status The HTTP status code. Defaults to `200 OK`.
         * @param headers A map of headers to merge with default headers.
         */
        CLUEAPI_INLINE
        base_response_t(
            body_t body,
            status_t::e_status status = status_t::ok,
            headers_t headers = {}) noexcept {
            m_body = std::move(body);

            {
                merge_headers(std::move(headers));

                m_headers.try_emplace("Content-Type", "text/plain");
            }

            m_status = status;
        }

       public:
        /**
         * @brief Adds a cookie to the response.
         *
         * @param cookie The `cookie_t` object to add.
         *
         * @return An `expected_t<void>` which is unexpected on serialization failure.
         */
        CLUEAPI_INLINE exceptions::expected_t<void> set_cookie(cookie_t cookie) noexcept {
            auto cookie_serialized = cookie.serialize();

            if (!cookie_serialized.has_value()) {
                return exceptions::make_unexpected(cookie_serialized.error());
            }

            m_cookies.emplace_back(std::move(cookie_serialized.value()));

            return exceptions::expected_t<void>{};
        }

        /**
         * @brief Resets the response object to a default state for reuse.
         */
        CLUEAPI_INLINE void reset() noexcept {
            m_body.clear();

            m_headers.clear();

            m_cookies.clear();

            m_status = status_t::ok;

            m_stream_fn = nullptr;

            m_is_stream = false;
        }

       protected:
        /**
         * @brief Merges a map of headers into the response's headers.
         *
         * @param headers The headers to merge. Existing headers with the same key will be
         * overwritten.
         */
        CLUEAPI_INLINE void merge_headers(headers_t&& headers) noexcept {
            m_headers = std::move(headers);
        }

       public:
        /**
         * @brief Gets a mutable reference to the response body.
         *
         * @return A reference to the body string.
         */
        CLUEAPI_INLINE auto& body() noexcept {
            return m_body;
        }

        /**
         * @brief Gets a const reference to the response body.
         *
         * @return A const reference to the body string.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& body() const noexcept {
            return m_body;
        }

        /**
         * @brief Gets a mutable reference to the response headers map.
         *
         * @return A reference to the headers map.
         */
        CLUEAPI_INLINE auto& headers() noexcept {
            return m_headers;
        }

        /**
         * @brief Gets a const reference to the response headers map.
         *
         * @return A const reference to the headers map.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& headers() const noexcept {
            return m_headers;
        }

        /**
         * @brief Gets a mutable reference to the vector of `Set-Cookie` strings.
         *
         * @return A reference to the cookies vector.
         */
        CLUEAPI_INLINE auto& cookies() noexcept {
            return m_cookies;
        }

        /**
         * @brief Gets a const reference to the vector of `Set-Cookie` strings.
         *
         * @return A const reference to the cookies vector.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& cookies() const noexcept {
            return m_cookies;
        }

        /**
         * @brief Gets a mutable reference to the HTTP status code.
         *
         * @return A reference to the status enum.
         */
        CLUEAPI_INLINE auto& status() noexcept {
            return m_status;
        }

        /**
         * @brief Gets a const reference to the HTTP status code.
         *
         * @return A const reference to the status enum.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& status() const noexcept {
            return m_status;
        }

        /**
         * @brief Gets a mutable reference to the streaming function.
         *
         * @return A reference to the `stream_fn_t` function object.
         */
        CLUEAPI_INLINE auto& stream_fn() noexcept {
            return m_stream_fn;
        }

        /**
         * @brief Gets a const reference to the streaming function.
         *
         * @return A const reference to the `stream_fn_t` function object.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& stream_fn() const noexcept {
            return m_stream_fn;
        }

        /**
         * @brief Gets a mutable reference to the stream flag.
         * @return A reference to the boolean flag indicating if this is a streaming response.
         */
        CLUEAPI_INLINE auto& is_stream() noexcept {
            return m_is_stream;
        }

        /**
         * @brief Gets a const reference to the stream flag.
         *
         * @return A const reference to the boolean flag indicating if this is a streaming response.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& is_stream() const noexcept {
            return m_is_stream;
        }

       public:
        /**
         * @brief Moves the body out of the response object.
         *
         * @return An rvalue reference to the response body.
         */
        CLUEAPI_INLINE auto&& move_body() && noexcept {
            return std::move(m_body);
        }

        /**
         * @brief Moves the headers out of the response object.
         *
         * @return An rvalue reference to the response headers.
         */
        CLUEAPI_INLINE auto&& move_headers() && noexcept {
            return std::move(m_headers);
        }

        /**
         * @brief Moves the cookies out of the response object.
         *
         * @return An rvalue reference to the response cookies.
         */
        CLUEAPI_INLINE auto&& move_cookies() && noexcept {
            return std::move(m_cookies);
        }

       protected:
        /**
         * @brief The response body.
         */
        body_t m_body{};

        /**
         * @brief The HTTP headers of the response.
         */
        headers_t m_headers{};

        /**
         * @brief The parsed cookies from the response.
         */
        resp_cookies_t m_cookies{};

        /**
         * @brief The HTTP status code of the response.
         */
        status_t::e_status m_status{};

        /**
         * @brief The streaming function for the response.
         */
        stream_fn_t m_stream_fn{};

        /**
         * @brief Whether the response is a streaming response.
         */
        bool m_is_stream{};
    };

    /**
     * @struct html_response_t
     *
     * @brief A convenience response class for `text/html` content.
     */
    struct html_response_t : base_response_t {
        CLUEAPI_INLINE constexpr html_response_t() noexcept = default;

        CLUEAPI_INLINE
        html_response_t(
            html_t body,
            status_t::e_status status = status_t::ok,
            headers_t headers = {}) noexcept {
            m_body = std::move(body);

            {
                merge_headers(std::move(headers));

                m_headers.try_emplace("Content-Type", "text/html");
            }

            m_status = status;
        }
    };

    /**
     * @struct redirect_response_t
     *
     * @brief A convenience response class for HTTP redirects.
     */
    struct redirect_response_t : base_response_t {
        CLUEAPI_INLINE constexpr redirect_response_t() noexcept = default;

        CLUEAPI_INLINE redirect_response_t(
            std::string location,
            status_t::e_status status = status_t::found,
            headers_t headers = {}) noexcept {
            m_body = "";

            if (status != status_t::moved_permanently && status != status_t::found &&
                status != status_t::see_other && status != status_t::temporary_redirect &&
                status != status_t::permanent_redirect)
                status = status_t::found;

            {
                merge_headers(std::move(headers));

                m_headers.emplace("Location", std::move(location));
                m_headers.emplace("Content-Type", "text/plain");
            }

            m_status = status;
        }
    };

    /**
     * @struct text_response_t
     *
     * @brief A convenience response class for `text/plain` content.
     */
    struct text_response_t : public base_response_t {
        CLUEAPI_INLINE constexpr text_response_t() noexcept = default;

        CLUEAPI_INLINE
        text_response_t(
            body_t body,
            status_t::e_status status = status_t::ok,
            headers_t headers = {}) noexcept {
            m_body = std::move(body);

            {
                merge_headers(std::move(headers));

                m_headers.try_emplace("Content-Type", "text/plain");
            }

            m_status = status;
        }
    };

    /**
     * @struct json_response_t
     *
     * @brief A convenience response class for `application/json` content.
     */
    struct json_response_t : public base_response_t {
        CLUEAPI_INLINE constexpr json_response_t() noexcept = default;

        CLUEAPI_INLINE
        json_response_t(
            const json_t::json_obj_t& body,
            status_t::e_status status = status_t::ok,
            headers_t headers = {}) noexcept {
            m_body = body.empty() ? "{}" : json_t::serialize(body);

            {
                merge_headers(std::move(headers));

                m_headers.try_emplace("Content-Type", "application/json");
            }

            m_status = status;
        }
    };

    /**
     * @struct file_response_t
     *
     * @brief A response class for streaming a file from disk.
     */
    struct file_response_t : public base_response_t {
        CLUEAPI_INLINE file_response_t() noexcept = default;

        file_response_t(
            boost::filesystem::path path,
            status_t::e_status status = status_t::ok,
            headers_t headers = {});
    };

    /**
     * @struct stream_response_t
     *
     * @brief A response class for custom, chunked-encoded streaming.
     */
    struct stream_response_t : public base_response_t {
        CLUEAPI_INLINE constexpr stream_response_t() noexcept = default;

        CLUEAPI_INLINE stream_response_t(
            stream_fn_t stream_fn,
            std::string content_type = "application/octet-stream",
            status_t::e_status status = status_t::ok,
            headers_t headers = {}) noexcept {
            m_stream_fn = std::move(stream_fn);

            merge_headers(std::move(headers));

            m_is_stream = true;

            {
                m_headers.emplace("Cache-Control", "no-cache");

                m_headers.emplace("Content-Type", std::move(content_type));
            }

            m_status = status;
        }
    };

    /**
     * @enum e_response_class
     *
     * @brief Defines the default class for error responses (e.g., plain text or JSON).
     */
    enum struct e_response_class : std::uint8_t { plain, json };

    /**
     * @struct response_class_t
     *
     * @brief A template-based factory for creating specific response types.
     *
     * @tparam _class_t The specific response class to create (e.g., `json_response_t`).
     * Must inherit from `base_response_t`.
     *
     * @details This utility simplifies the creation of error responses by allowing the default
     * error response type (e.g., plain text vs. JSON) to be configured globally.
     * The static `make` function provides a uniform interface for constructing different
     * response types.
     */
    template <typename _class_t = base_response_t>
    struct response_class_t {
        static_assert(
            std::is_base_of_v<base_response_t, _class_t>,

            "Class must inherit from response_t");

        static_assert(
            std::is_same_v<base_response_t, std::decay_t<_class_t>> ||
                std::is_same_v<json_response_t, std::decay_t<_class_t>>,

            "Class must not be response_t or json_response_t");

       public:
        CLUEAPI_INLINE constexpr response_class_t() noexcept = default;

        /**
         * @brief Creates an instance of the specified response class.
         *
         * @tparam _body_t The type of the response body.
         *
         * @param body The content for the response body.
         * @param status_code The HTTP status code for the response.
         * @param headers Any additional headers for the response.
         *
         * @return An instance of the specified response class `_class_t`.
         */
        template <typename _body_t>
        CLUEAPI_INLINE static _class_t make(
            _body_t&& body,

            status_t::e_status status_code = status_t::ok,

            headers_t headers = {}) noexcept {
            return _class_t(
                std::forward<_body_t>(body),

                std::move(status_code),

                std::move(headers));
        }
    };

    /**
     * @brief The primary type alias for a response, defaulting to `base_response_t`.
     */
    using response_t = base_response_t;
} // namespace clueapi::http::types

#endif // CLUEAPI_HTTP_TYPES_RESPONSE_HXX