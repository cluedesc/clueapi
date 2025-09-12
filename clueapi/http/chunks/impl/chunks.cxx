/**
 * @file chunks.cxx
 *
 * @brief Implements the chunk writer.
 */

#include <clueapi.hxx>

namespace clueapi::http::chunks {
    exceptions::expected_awaitable_t<> chunk_writer_t::write_chunk(std::string_view data) noexcept {
        m_buffer.clear();

        fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{:X}"), data.size());

        m_buffer.append(k_crlf, k_crlf + k_crlf_size);

#ifndef _WIN32
        m_buffer.append(data.begin(), data.end());
#else
        m_buffer.append(data);
#endif // _WIN32

        m_buffer.append(k_crlf, k_crlf + k_crlf_size);

        co_return co_await exceptions::wrap_awaitable(
            boost::asio::async_write(
                m_socket,
                boost::asio::buffer(m_buffer.data(), m_buffer.size()),
                boost::asio::use_awaitable),

            exceptions::io_error_t::make("Failed to write chunk"));
    }

    exceptions::expected_awaitable_t<> chunk_writer_t::write_final_chunk() noexcept {
        if (m_final_chunk_written)
            co_return exceptions::expected_t<>{};

        m_final_chunk_written = true;

        co_return co_await exceptions::wrap_awaitable(
            boost::asio::async_write(
                m_socket, boost::asio::buffer(k_final_chunk), boost::asio::use_awaitable),

            exceptions::io_error_t::make("Failed to write final chunk"));
    }
} // namespace clueapi::http::chunks