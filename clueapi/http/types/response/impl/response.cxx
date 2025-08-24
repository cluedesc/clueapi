/**
 * @file response.cxx
 *
 * @brief Implements the response type.
 */

#include <clueapi.hxx>

namespace clueapi::http::types {
    inline constexpr std::size_t k_def_buffer_size = 8192;

    file_response_t::file_response_t(
        boost::filesystem::path path, status_t::e_status status, headers_t headers) {
        try {
            if (!boost::filesystem::exists(path) || !boost::filesystem::is_regular_file(path)) {
                m_status = status_t::not_found;

                return;
            }

            m_is_stream = true;

            merge_headers(std::move(headers));

            const auto file_size = boost::filesystem::file_size(path);

            auto content_type = std::string{mime::mime_t::mime_type(path)};

            {
                m_headers.try_emplace("Content-Type", std::move(content_type));

                m_headers.try_emplace(
                    "Content-Length", boost::lexical_cast<std::string>(file_size));

                {
                    auto last_write = boost::filesystem::last_write_time(path);
                    auto etag_value = fmt::format("\"{}-{}\"", last_write, file_size);

                    m_headers.emplace("ETag", std::move(etag_value));
                }
            }

            m_status = status;

            m_stream_fn =
                [file_size, path = std::move(path)](
                    chunks::chunk_writer_t& writer) -> exceptions::expected_awaitable_t<> {
                auto executor = co_await boost::asio::this_coro::executor;

                boost::asio::stream_file file(
                    executor, path.string(), boost::asio::stream_file::read_only);

                if (!file.is_open()) {
                    co_return exceptions::make_unexpected(
                        exceptions::io_error_t::make("Can't open file: {}", path.string()));
                }

                std::array<char, k_def_buffer_size> buffer{};

                std::size_t total_sent{};

                while (total_sent < file_size && !writer.writer_closed()) {
                    const auto to_read =
                        std::min<std::size_t>(k_def_buffer_size, file_size - total_sent);

                    const auto read_size = co_await file.async_read_some(
                        boost::asio::buffer(buffer.data(), to_read), boost::asio::use_awaitable);

                    if (read_size <= 0)
                        break;

                    total_sent += read_size;

                    auto result = co_await exceptions::wrap_awaitable(
                        writer.write_chunk(std::string_view{buffer.data(), read_size}),

                        exceptions::io_error_t::make("Failed to write chunk"));

                    if (!result.has_value()) {
                        if (file.is_open())
                            file.close();

                        co_return exceptions::make_unexpected(result.error());
                    }
                }

                file.close();

                co_return exceptions::expected_t<>{};
            };
        } catch (const std::exception& e) {
            throw exceptions::exception_t{"Failed to initialize file response: {}", e.what()};
        } catch (...) {
            throw exceptions::exception_t{"Failed to initialize file response: unknown"};
        }
    }
} // namespace clueapi::http::types