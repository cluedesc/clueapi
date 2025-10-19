/**
 * @file ctx.hxx
 *
 * @brief Defines the request context object used by route handlers.
 */

#ifndef CLUEAPI_HTTP_CTX_HXX
#define CLUEAPI_HTTP_CTX_HXX

#include <utility>
#include <string_view>

#include <boost/algorithm/string/find.hpp>
#include <boost/filesystem/path.hpp>

#include "clueapi/exceptions/exceptions.hxx"

#include "clueapi/http/multipart/multipart.hxx"
#include "clueapi/http/types/field/field.hxx"
#include "clueapi/http/types/file/file.hxx"
#include "clueapi/http/types/request/request.hxx"

#include "clueapi/shared/macros.hxx"
#include "clueapi/shared/non_copy/extract_from/extract_from.hxx"
#include "clueapi/shared/shared.hxx"

#include "clueapi/modules/macros.hxx"

namespace clueapi::http {
    /**
     * @struct ctx_t
     *
     * @brief Encapsulates all data related to an incoming HTTP request for a route handler.
     *
     * @details This object is the primary interface for a route handler to interact with a request.
     * It provides access to the raw request, URL parameters, and parsed multipart/form-data
     * fields and files. It is constructed by the framework and passed to the user-defined handler.
     */
    struct ctx_t {
        CLUEAPI_INLINE ctx_t() noexcept = default;

        /**
         * @brief Constructs a context with a request and URL parameters.
         *
         * @param request The HTTP request object.
         * @param params The map of URL parameters extracted by the router.
         */
        CLUEAPI_INLINE ctx_t(types::request_t request, types::params_t params) noexcept
            : m_params(std::move(params)), m_request(std::move(request)) {
        }

        CLUEAPI_INLINE ~ctx_t() = default;

       public:
        CLUEAPI_INLINE ctx_t(const ctx_t&) = delete;

        CLUEAPI_INLINE ctx_t& operator=(const ctx_t&) = delete;

        CLUEAPI_INLINE ctx_t(ctx_t&&) noexcept = default;

        CLUEAPI_INLINE ctx_t& operator=(ctx_t&&) noexcept = default;

       public:
        /**
         * @brief Asynchronously creates and fully parses a request context.
         *
         * @param request The raw HTTP request object.
         * @param params URL parameters matched by the router.
         * @param cfg Configuration for the multipart parser.
         *
         * @return An awaitable that resolves to a fully initialized `ctx_t`.
         *
         * @details This is the primary factory for creating a `ctx_t`. It performs the necessary
         * asynchronous parsing of the request body (e.g., multipart/form-data) before the
         * context is passed to a handler.
         */
        CLUEAPI_NOINLINE static shared::awaitable_t<ctx_t> make_awaitable(
            types::request_t request,
            types::params_t params,
            multipart::parser_t::cfg_t cfg) {
            ctx_t ctx{std::move(request), std::move(params)};

            co_await ctx.parse(cfg);

            co_return ctx;
        }

       public:
        /**
         * @brief Gets the URL parameters.
         *
         * @return A reference to the map of URL parameters.
         */
        CLUEAPI_INLINE auto& params() noexcept {
            return m_params;
        }

        /**
         * @brief Gets the URL parameters.
         *
         * @return A const reference to the map of URL parameters.
         */
        CLUEAPI_INLINE const auto& params() const noexcept {
            return m_params;
        }

        /**
         * @brief Gets the uploaded files from a multipart request.
         *
         * @return A reference to the map of files.
         */
        CLUEAPI_INLINE auto& files() noexcept {
            return m_files;
        }

        /**
         * @brief Gets the uploaded files from a multipart request.
         *
         * @return A const reference to the map of files.
         */
        CLUEAPI_INLINE const auto& files() const noexcept {
            return m_files;
        }

        /**
         * @brief Gets the form fields from a multipart request.
         *
         * @return A reference to the map of fields.
         */
        CLUEAPI_INLINE auto& fields() noexcept {
            return m_fields;
        }

        /**
         * @brief Gets the form fields from a multipart request.
         *
         * @return A const reference to the map of fields.
         */
        CLUEAPI_INLINE const auto& fields() const noexcept {
            return m_fields;
        }

        /**
         * @brief Gets the underlying HTTP request object.
         *
         * @return A reference to the request object.
         */
        CLUEAPI_INLINE auto& request() noexcept {
            return m_request;
        }

        /**
         * @brief Gets the underlying HTTP request object.
         *
         * @return A const reference to the request object.
         */
        CLUEAPI_INLINE const auto& request() const noexcept {
            return m_request;
        }

       private:
        /**
         * @brief Parses the request body using a multipart parser.
         *
         * @param cfg The configuration for the multipart parser.a
         *
         * @return An awaitable that resolves to void.
         */
        CLUEAPI_NOINLINE shared::awaitable_t<void> parse(multipart::parser_t::cfg_t cfg) {
            const auto content_type_it = m_request.headers().find("content-type");

            if (content_type_it == m_request.headers().end())
                co_return;

            const auto content_type = content_type_it->second;

            const auto boundary = shared::non_copy::extract_str(content_type, "boundary");

            if (boundary.empty())
                co_return;

            cfg.m_boundary = boundary;

            if (!m_request.parse_path().empty()) {
                co_await parse_file_multipart(cfg, m_request.parse_path());
            } else
                co_await parse_body_multipart(cfg, m_request.body(), content_type);

            co_return;
        }

        /**
         * @brief Parses a file part using the multipart parser.
         *
         * @param cfg The multipart parser configuration.
         * @param path The path to the file to parse.
         *
         * @return An awaitable that resolves to void.
         */
        CLUEAPI_NOINLINE shared::awaitable_t<void> parse_file_multipart(
            multipart::parser_t::cfg_t cfg, const boost::filesystem::path& path) {
            auto parser = multipart::parser_t{cfg};

            auto result = co_await parser.parse_file(path);

            boost::system::error_code ec{};

            boost::filesystem::remove(path, ec);

            if (ec) {
                CLUEAPI_LOG_WARNING(exceptions::exception_t::make(
                    "Failed to delete temp file (path: {}): {}", path.string(), ec.what()));
            }

            if (!result.has_value()) {
                CLUEAPI_LOG_WARNING(exceptions::exception_t::make(
                    "Failed to parse multipart body: {}", result.error()));

                co_return;
            }

            auto&& parsed_parts = std::move(result.value());

            m_fields = std::move(parsed_parts.m_fields);
            m_files = std::move(parsed_parts.m_files);

            co_return;
        }

        /**
         * @brief Parses a body part using the multipart parser.
         *
         * @param cfg The multipart parser configuration.
         * @param body The body to parse.
         * @param content_type The content type of the body.
         *
         * @return An awaitable that resolves to void.
         */
        CLUEAPI_NOINLINE shared::awaitable_t<void> parse_body_multipart(
            multipart::parser_t::cfg_t cfg, std::string_view body, std::string_view content_type) {
            if (!boost::ifind_first(content_type, "multipart/form-data"))
                co_return;

            auto parser = multipart::parser_t{cfg};

            auto result = co_await parser.parse(body);

            if (!result.has_value()) {
                CLUEAPI_LOG_WARNING(exceptions::exception_t::make(
                    "Failed to parse multipart body: {}", result.error()));

                co_return;
            }

            auto&& parsed_parts = std::move(result.value());

            m_fields = std::move(parsed_parts.m_fields);
            m_files = std::move(parsed_parts.m_files);

            co_return;
        }

       private:
        /**
         * @brief The URL parameters extracted from the request.
         */
        types::params_t m_params;

        /**
         * @brief The uploaded files from a multipart request.
         */
        types::files_t m_files;

        /**
         * @brief The form fields from a multipart request.
         */
        types::fields_t m_fields;

        /**
         * @brief The raw HTTP request.
         */
        types::request_t m_request;
    };
} // namespace clueapi::http

#endif // CLUEAPI_HTTP_CTX_HXX