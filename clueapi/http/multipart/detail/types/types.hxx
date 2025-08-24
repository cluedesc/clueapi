/**
 * @file detail.hxx
 *
 * @brief A file for defining the details of the multipart parser.
 */

#ifndef CLUEAPI_HTTP_MULTIPART_DETAIL_TYPES_HXX
#define CLUEAPI_HTTP_MULTIPART_DETAIL_TYPES_HXX

namespace clueapi::http::multipart::detail {
    /**
     * @struct cfg_t
     *
     * @brief Configuration settings for the multipart parser.
     */
    struct cfg_t {
        /**
         * @brief The boundary string used to separate parts of the multipart message.
         *
         * @note This is typically extracted from the Content-Type header and set internally.
         */
        std::string_view m_boundary;

        /**
         * @brief The size of the buffer chunk used when reading from a file stream.
         */
        std::size_t m_chunk_size{65536};

        /**
         * @brief The maximum size (in bytes) a single uploaded file can have before
         * it is written to a temporary file on disk.
         */
        std::size_t m_max_file_size_in_memory{1048576};

        /**
         * @brief The cumulative maximum size (in bytes) of all uploaded files that can be
         * stored in memory.
         *
         * @details If this limit is exceeded, subsequent file uploads will be streamed to
         * temporary files.
         */
        std::size_t m_max_files_size_in_memory{1048576u * 10u};

        /**
         * @brief The maximum number of parts (files + fields) allowed in a single request to
         * prevent abuse.
         */
        std::size_t m_max_parts_count{1024};
    };

    /**
     * @struct parts_t
     *
     * @brief Contains the results of a successful parse operation.
     */
    struct parts_t {
        CLUEAPI_INLINE parts_t() noexcept = default;

       public:
        /**
         * @brief The uploaded files from the multipart request.
         */
        types::files_t m_files;

        /**
         * @brief The form fields from the multipart request.
         */
        types::fields_t m_fields;
    };

    /**
     * @struct tmp_file_t
     *
     * @brief A RAII wrapper for a file that is read.
     */
    struct tmp_file_t {
        /**
         * @brief Constructs a temporary file wrapper.
         *
         * @param executor The I/O executor.
         * @param path The path to the temporary file.
         */
        CLUEAPI_INLINE tmp_file_t(
            const boost::asio::any_io_executor& executor, boost::filesystem::path path) noexcept
            : m_path(std::move(path)), m_file(executor) {
        }

        /**
         * @brief Destroys the temporary file wrapper, ensuring the file is closed.
         */
        CLUEAPI_INLINE ~tmp_file_t() noexcept {
            boost::system::error_code ec{};

            if (!m_file.is_open())
                return;

            m_file.close(ec);
        }

        // Move constructor
        CLUEAPI_INLINE tmp_file_t(tmp_file_t&&) noexcept = default;

        // Move assignment operator
        CLUEAPI_INLINE tmp_file_t& operator=(tmp_file_t&&) noexcept = default;

        // Copy constructor
        CLUEAPI_INLINE tmp_file_t(const tmp_file_t&) = delete;

        // Copy assignment operator
        CLUEAPI_INLINE tmp_file_t& operator=(const tmp_file_t&) = delete;

       public:
        /**
         * @brief Gets the underlying file object.
         *
         * @return A reference to the stream file.
         */
        CLUEAPI_INLINE auto& file_obj() noexcept {
            return m_file;
        }

        /**
         * @brief Gets the underlying file object.
         *
         * @return A const reference to the stream file.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& file_obj() const noexcept {
            return m_file;
        }

        /**
         * @brief Gets the path to the temporary file.
         *
         * @return A reference to the path.
         */
        CLUEAPI_INLINE auto& path() noexcept {
            return m_path;
        }

        /**
         * @brief Gets the path to the temporary file.
         *
         * @return A const reference to the path.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& path() const noexcept {
            return m_path;
        }

       private:
        /**
         * @brief The path to the temporary file.
         */
        boost::filesystem::path m_path;

        /**
         * @brief The Boost.Asio stream file object.
         */
        boost::asio::stream_file m_file;
    };
} // namespace clueapi::http::multipart::detail

#endif // CLUEAPI_HTTP_MULTIPART_DETAIL_TYPES_HXX