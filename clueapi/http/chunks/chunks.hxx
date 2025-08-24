/**
 * @file chunks.hxx
 *
 * @brief Defines a utility for writing data using HTTP chunked transfer encoding.
 */

#ifndef CLUEAPI_HTTP_CHUNKS_HXX
#define CLUEAPI_HTTP_CHUNKS_HXX

/**
 * @namespace clueapi::http::chunks
 *
 * @brief The main namespace for the clueapi HTTP chunked data writer.
 */
namespace clueapi::http::chunks {
    /**
     * @brief The default buffer size for the chunk writer's internal memory buffer.
     */
    inline constexpr std::size_t k_def_buffer_size = 1024;

    /**
     * @struct chunk_writer_t
     *
     * @brief Manages the process of sending data in HTTP chunked transfer encoding format over a
     * socket.
     *
     * @details This class formats data into the required chunked structure (size, CRLF, data, CRLF)
     * and writes it asynchronously to a TCP socket. It is essential for streaming responses where
     * the total content length is not known in advance.
     */
    struct chunk_writer_t {
        /**
         * @brief Constructs a chunk writer.
         *
         * @param socket The TCP socket to write the chunked data to.
         * The socket must remain valid for the lifetime of the writer.
         */
        CLUEAPI_INLINE chunk_writer_t(boost::asio::ip::tcp::socket& socket) noexcept
            : m_socket{socket} {
        }

        CLUEAPI_INLINE ~chunk_writer_t() noexcept = default;

       public:
        /**
         * @brief Asynchronously writes a single chunk of data to the socket.
         *
         * @param data A `std::string_view` representing the data to be sent in this chunk.
         *
         * @return An `exceptions::expected_awaitable_t<>` that resolves to an empty expected on
         * success, or an unexpected containing an error message on failure.
         *
         * @details The function formats the data by prepending its size in hexadecimal followed by
         * CRLF, and appending a final CRLF.
         */
        exceptions::expected_awaitable_t<> write_chunk(std::string_view data) noexcept;

        /**
         * @brief Asynchronously writes the final zero-sized chunk to signify the end of the
         * response body.
         *
         * @return An `exceptions::expected_awaitable_t<>` that resolves to an empty expected on
         * success, or an unexpected containing an error message on failure.
         *
         * @note This must be called exactly once after all data chunks have been written.
         */
        exceptions::expected_awaitable_t<> write_final_chunk() noexcept;

       public:
        /**
         * @brief Checks if the underlying socket is closed.
         *
         * @return `true` if the socket is no longer open, `false` otherwise.
         */
        [[nodiscard]] CLUEAPI_INLINE bool writer_closed() const noexcept {
            return !m_socket.is_open();
        }

        /**
         * @brief Checks if the final chunk has been written.
         *
         * @return `true` if `write_final_chunk()` has been successfully called, `false` otherwise.
         */
        [[nodiscard]] CLUEAPI_INLINE bool final_chunk_written() const noexcept {
            return m_final_chunk_written;
        }

       private:
        /**
         * @brief Flag indicating if the final chunk has been written.
         */
        bool m_final_chunk_written{false};

        /**
         * @brief The socket to write to.
         */
        boost::asio::ip::tcp::socket& m_socket;

        /**
         * @brief The buffer for storing chunked data.
         *
         * @details This is a basic_memory_buffer with a default capacity of `k_def_buffer_size`
         * bytes.
         */
        fmt::basic_memory_buffer<char, k_def_buffer_size> m_buffer;

       private:
        /**
         * @brief The CRLF sequence.
         *
         * @details This is a static constant array of two characters, representing the CRLF
         * sequence.
         */
        static constexpr char k_crlf[2] = {'\r', '\n'};

        /**
         * @brief The size of the CRLF sequence.
         *
         * @details This is the size of the `k_crlf` array.
         */
        static constexpr std::size_t k_crlf_size = std::ranges::size(k_crlf);

        /**
         * @brief The final chunk sequence.
         *
         * @details This is a static constant string_view representing the final chunk sequence.
         */
        static constexpr std::string_view k_final_chunk = "0\r\n\r\n";

        /**
         * @brief The size of the final chunk sequence.
         *
         * @details This is the size of the `k_final_chunk` string_view.
         */
        static constexpr std::size_t k_final_chunk_size = std::ranges::size(k_final_chunk);
    };
} // namespace clueapi::http::chunks

#endif // CLUEAPI_HTTP_CHUNKS_HXX