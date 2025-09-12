/**
 * @file file.hxx
 *
 * @brief Defines the representation of an uploaded file from a multipart request.
 */

#ifndef CLUEAPI_HTTP_TYPES_FILE_HXX
#define CLUEAPI_HTTP_TYPES_FILE_HXX

namespace clueapi::http::types {
    /**
     * @struct file_t
     *
     * @brief Represents a single file uploaded via a `multipart/form-data` request.
     *
     * @details This class is a move-only type that manages the lifetime of an uploaded file's data.
     * The file's contents can be stored either in an in-memory buffer (`std::vector<char>`) for
     * small files or as a path to a temporary file on disk for large files. The destructor
     * ensures that any temporary file created on disk is automatically deleted.
     */
    struct file_t {
        CLUEAPI_INLINE constexpr file_t() noexcept = default;

        /**
         * @brief Constructs an in-memory file.
         *
         * @param name The original filename provided by the client.
         * @param content_type The MIME type of the file.
         * @param data A vector of bytes containing the file's content.
         */
        CLUEAPI_INLINE file_t(
            std::string name, std::string content_type, std::vector<char> data) noexcept
            : m_name(std::move(name)),
              m_content_type(std::move(content_type)),
              m_data(std::move(data)),
              m_in_memory(true) {
        }

        /**
         * @brief Constructs a file stored on disk.
         *
         * @param name The original filename provided by the client.
         * @param content_type The MIME type of the file.
         * @param temp_path The path to the temporary file on disk.
         */
        CLUEAPI_INLINE file_t(
            std::string name, std::string content_type, boost::filesystem::path temp_path) noexcept
            : m_name(std::move(name)),
              m_content_type(std::move(content_type)),
              m_temp_path(std::move(temp_path)) {
        }

        /**
         * @brief Destroys the file object.
         *
         * @details This function destroys the file object by either deleting the temporary file
         * on disk or clearing the in-memory data. It also resets the object to its default state.
         */
        CLUEAPI_INLINE ~file_t() noexcept {
            if (m_in_memory || m_temp_path.empty())
                return;

            boost::system::error_code ec{};

            boost::filesystem::remove(m_temp_path, ec);
        }

        // Move constructor
        CLUEAPI_INLINE file_t(file_t&& other) noexcept {
            m_name = std::move(other.m_name);

            m_content_type = std::move(other.m_content_type);

            m_data = std::move(other.m_data);

            m_temp_path = std::move(other.m_temp_path);

            m_in_memory = other.m_in_memory;

            other.reset_ownership();
        }

        // Move assignment operator
        CLUEAPI_INLINE file_t& operator=(file_t&& other) noexcept {
            if (this != &other) {
                if (!m_in_memory && !m_temp_path.empty()) {
                    boost::system::error_code error_code{};

                    boost::filesystem::remove(m_temp_path, error_code);
                }

                m_name = std::move(other.m_name);

                m_content_type = std::move(other.m_content_type);

                m_data = std::move(other.m_data);

                m_temp_path = std::move(other.m_temp_path);

                m_in_memory = other.m_in_memory;

                other.reset_ownership();
            }

            return *this;
        }

       public:
        /**
         * @brief Gets the size of the file in bytes.
         *
         * @return The file size. Returns 0 if the file does not exist or an error occurs.
         */
        [[nodiscard]] CLUEAPI_INLINE std::size_t size() const noexcept {
            if (m_in_memory)
                return m_data.size();

            if (!m_temp_path.empty() && boost::filesystem::exists(m_temp_path)) {
                boost::system::error_code error_code{};

                const auto sz = boost::filesystem::file_size(m_temp_path, error_code);

                return error_code ? 0 : sz;
            }

            return 0;
        }

       private:
        /**
         * @brief Resets ownership of the file object.
         *
         * @details This function resets the ownership of the file object by clearing the temporary
         * file path and setting the object to use in-memory storage. It also resets the file data,
         * name, content type, and other properties.
         */
        CLUEAPI_INLINE void reset_ownership() noexcept {
            m_temp_path.clear();

            m_in_memory = true;

            m_data.clear();
            m_name.clear();

            m_content_type.clear();
        }

       public:
        /**
         * @brief Gets the original filename as provided by the client.
         *
         * @return A const reference to the filename string.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& name() const noexcept {
            return m_name;
        }

        /**
         * @brief Gets the safe filename as provided by the client.
         *
         * @return A safe filename string.
         */
        [[nodiscard]] CLUEAPI_INLINE auto safe_name() const noexcept {
            return shared::sanitize_filename(m_name);
        }

        /**
         * @brief Gets the MIME type of the file.
         *
         * @return A const reference to the content type string.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& content_type() const noexcept {
            return m_content_type;
        }

        /**
         * @brief Gets the path to the temporary file on disk.
         *
         * @return A const reference to the `boost::filesystem::path`. Will be empty if the file is
         * in memory.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& temp_path() const noexcept {
            return m_temp_path;
        }

        /**
         * @brief Gets the file's content as a byte vector.
         *
         * @return A const reference to the data vector. Will be empty if the file is stored on
         * disk.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& data() const noexcept {
            return m_data;
        }

        /**
         * @brief Checks if the file is stored in memory.
         *
         * @return `true` if the file content is in memory, `false` if it is on disk.
         */
        [[nodiscard]] CLUEAPI_INLINE const auto& in_memory() const noexcept {
            return m_in_memory;
        }

       private:
        /**
         * @brief The name of the file.
         */
        std::string m_name{};

        /**
         * @brief The MIME type of the file.
         */
        std::string m_content_type{};

        /**
         * @brief The path to the temporary file on disk.
         */
        boost::filesystem::path m_temp_path{};

        /**
         * @brief The file's content as a byte vector.
         */
        std::vector<char> m_data{};

        /**
         * @brief Whether the file is stored in memory.
         */
        bool m_in_memory{};
    };

    /**
     * @brief Type alias for a map of uploaded files from a `multipart/form-data` request.
     *
     * @details The key is the field name from the form, and the value is the `file_t` object.
     */
    using files_t = shared::
        unordered_map_t<std::string, file_t, http::detail::sv_hash_t, http::detail::sv_eq_t>;
} // namespace clueapi::http::types

#endif // CLUEAPI_HTTP_TYPES_FILE_HXX