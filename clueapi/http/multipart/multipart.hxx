/**
 * @file multipart.hxx
 *
 * @brief Defines the parser for `multipart/form-data` content.
 */

#ifndef CLUEAPI_HTTP_MULTIPART_HXX
#define CLUEAPI_HTTP_MULTIPART_HXX

#include "detail/detail.hxx"

/**
 * @namespace clueapi::http::multipart
 *
 * @brief The main namespace for the clueapi HTTP multipart parser.
 */
namespace clueapi::http::multipart {
    /**
     * @struct parser_t
     *
     * @brief The main multipart parser class.
     */
    struct parser_t {
        /**
         * @brief A type alias for the configuration struct.
         */
        using cfg_t = detail::cfg_t;

        /**
         * @brief A type alias for the parsed parts struct.
         */
        using parts_t = detail::parts_t;

       public:
        /**
         * @brief Constructs a multipart parser with the given configuration.
         *
         * @param cfg The configuration struct.
         */
        CLUEAPI_INLINE parser_t(cfg_t cfg) noexcept : m_cfg(cfg) {
            m_dash_boundary = fmt::format(FMT_COMPILE("--{}"), m_cfg.m_boundary);

            m_crlf_dash_boundary = fmt::format(FMT_COMPILE("\r\n{}"), m_dash_boundary);
        }

       public:
        /**
         * @brief Asynchronously parses a multipart body from an in-memory string view.
         *
         * @param body The string view containing the multipart body.
         */
        CLUEAPI_NOINLINE exceptions::expected_awaitable_t<parts_t> parse(std::string_view body) {
            auto parser =
                detail::string_parser_t{body, m_cfg, m_dash_boundary, m_crlf_dash_boundary};

            auto result = co_await parser.parse();

            co_return exceptions::expected_t<parts_t>{std::move(result)};
        }

        /**
         * @brief Asynchronously parses a multipart body by streaming it from a file.
         *
         * @param file_path The path to the file to stream.
         */
        CLUEAPI_NOINLINE exceptions::expected_awaitable_t<parts_t> parse_file(
            const boost::filesystem::path& file_path) {
            const auto& executor = co_await boost::asio::this_coro::executor;

            auto parser = detail::file_parser_t{
                executor, file_path, m_cfg, m_dash_boundary, m_crlf_dash_boundary};

            auto result = co_await parser.parse();

            co_return exceptions::expected_t<parts_t>{std::move(result)};
        }

       private:
        /**
         * @brief The configuration for the multipart parser.
         */
        cfg_t m_cfg;

        /**
         * @brief The dash boundary string.
         */
        std::string m_dash_boundary;

        /**
         * @brief The CRLF dash boundary string.
         */
        std::string m_crlf_dash_boundary;
    };
} // namespace clueapi::http::multipart

#endif // CLUEAPI_HTTP_MULTIPART_HXX