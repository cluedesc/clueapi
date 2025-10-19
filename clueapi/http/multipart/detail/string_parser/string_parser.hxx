/**
 * @file string_parser.hxx
 *
 * @brief A file for defining the details of the multipart parser.
 */

#ifndef CLUEAPI_HTTP_MULTIPART_DETAIL_STRING_PARSER_HXX
#define CLUEAPI_HTTP_MULTIPART_DETAIL_STRING_PARSER_HXX

#include <string_view>
#include <utility>
#include <vector>

#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/filesystem/path.hpp>

#include "clueapi/http/multipart/detail/base_parser/base_parser.hxx"
#include "clueapi/http/multipart/detail/headers_parser/headers_parser.hxx"
#include "clueapi/http/multipart/detail/types/types.hxx"

/**
 * @namespace clueapi::http::multipart::detail
 *
 * @brief Defines the details of the multipart parser.
 */
namespace clueapi::http::multipart::detail {
    /**
     * @struct string_parser_t
     *
     * @brief The string parser class.
     *
     * @details This class is responsible for parsing a string part of a multipart message.
     */
    struct string_parser_t : public base_parser_t {
        /**
         * @brief Constructs a string parser.
         *
         * @param str The string to parse.
         * @param cfg The configuration for the multipart parser.
         * @param dash_boundary A string_view containing the dash boundary string.
         * @param crlf_dash_boundary A string_view containing the CRLF dash boundary string.
         */
        CLUEAPI_INLINE string_parser_t(
            std::string_view str,
            cfg_t cfg,
            std::string_view dash_boundary,
            std::string_view crlf_dash_boundary) noexcept
            : m_cfg(cfg),
              m_str(str),
              m_dash_boundary(dash_boundary),
              m_crlf_dash_boundary(crlf_dash_boundary) {
        }

       public:
        /**
         * @brief Parses the string part.
         *
         * @return An awaitable that resolves to a `parts_t` object containing the parsed parts.
         *
         * @details This function parses the string part by removing the dash boundary from the
         * string and then iterating over the string, parsing each part individually. It returns a
         * `parts_t` object containing the parsed parts.
         */
        CLUEAPI_NOINLINE exceptions::expected_awaitable_t<parts_t> parse() override {
            if (!m_str.starts_with(m_dash_boundary)) {
                co_return exceptions::make_unexpected(
                    exceptions::runtime_error_t::make("Body does not start with boundary"));
            }

            m_str.remove_prefix(m_dash_boundary.length());

            std::size_t iteration_count{};

            constexpr std::size_t k_max_iterations = 100000;

            parts_t result{};

            while (iteration_count++ < k_max_iterations) {
                if (result.m_fields.size() + result.m_files.size() >= m_cfg.m_max_parts_count) {
                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Maximum number of parts exceeded"));
                }

                if (m_str.starts_with("--"))
                    co_return exceptions::expected_t<parts_t>{std::move(result)};

                if (!m_str.starts_with("\r\n")) {
                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Malformed boundary line"));
                }

                m_str.remove_prefix(2);

                const auto headers_end_pos = m_str.find("\r\n\r\n");

                if (headers_end_pos == std::string_view::npos) {
                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Can't find headers end section"));
                }

                auto headers_blob = m_str.substr(0, headers_end_pos);

                auto headers = detail::parse_headers(headers_blob);

                m_str.remove_prefix(headers_end_pos + 4);

                auto content_end_pos = m_str.find(m_crlf_dash_boundary);

                if (content_end_pos == std::string_view::npos) {
                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Can't find content end section"));
                }

                const auto content = m_str.substr(0, content_end_pos);

                if (!headers.m_name.empty()) {
                    const auto content_len = content.length();

                    const auto is_file = !headers.m_file_name.empty();

                    auto executor = co_await boost::asio::this_coro::executor;

                    if (is_file) {
                        if (content_len > m_cfg.m_max_file_size_in_memory ||
                            m_total_in_memory_size + content_len >
                                m_cfg.m_max_files_size_in_memory) {
                            boost::system::error_code ec{};

                            auto temp_path =
                                boost::filesystem::temp_directory_path() /
                                boost::filesystem::unique_path("clueapi-%%%%-%%%%.tmp");

                            detail::tmp_file_t new_storage{executor, temp_path};

                            new_storage.file_obj().open(
                                temp_path.string(),
                                boost::asio::stream_file::write_only |
                                    boost::asio::stream_file::create,
                                ec);

                            if (ec) {
                                co_return exceptions::make_unexpected(exceptions::io_error_t::make(
                                    "Failed to create temp file: {}", ec.message()));
                            }

                            co_await boost::asio::async_write(
                                new_storage.file_obj(),
                                boost::asio::buffer(content),
                                boost::asio::use_awaitable);

                            result.m_files.emplace(
                                headers.m_name,

                                types::file_t{
                                    std::move(headers.m_file_name),
                                    std::move(headers.m_content_type),
                                    std::move(temp_path)});
                        } else {
                            m_total_in_memory_size += content_len;

                            if (content_len > m_cfg.m_chunk_size)
                                co_await boost::asio::post(executor, boost::asio::use_awaitable);

                            std::vector<char> data(content.begin(), content.end());

                            result.m_files.emplace(
                                headers.m_name,

                                types::file_t{
                                    std::move(headers.m_file_name),
                                    std::move(headers.m_content_type),
                                    std::move(data)});
                        }
                    } else {
                        result.m_fields.emplace(
                            headers.m_name,

                            std::string{content.begin(), content.end()});
                    }
                }

                m_str.remove_prefix(content_end_pos + m_crlf_dash_boundary.length());
            }

            co_return exceptions::make_unexpected(
                "Maximum iterations exceeded - possible infinite loop");
        }

       private:
        /**
         * @brief The configuration for the multipart parser.
         */
        cfg_t m_cfg;

        /**
         * @brief The string to parse.
         */
        std::string_view m_str;

        /**
         * @brief The total size of the part in memory.
         */
        std::size_t m_total_in_memory_size{0};

        /**
         * @brief The dash boundary string.
         */
        std::string_view m_dash_boundary;

        /**
         * @brief The CRLF dash boundary string.
         */
        std::string_view m_crlf_dash_boundary;
    };
} // namespace clueapi::http::multipart::detail

#endif // CLUEAPI_HTTP_MULTIPART_DETAIL_STRING_PARSER_HXX