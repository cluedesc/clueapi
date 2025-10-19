/**
 * @file detail.hxx
 *
 * @brief A file for defining the details of the multipart parser.
 */

#ifndef CLUEAPI_HTTP_MULTIPART_DETAIL_TYPES_HXX
#define CLUEAPI_HTTP_MULTIPART_DETAIL_TYPES_HXX

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/system/error_code.hpp>

#include "clueapi/http/types/file/file.hxx"
#include "clueapi/http/types/field/field.hxx"

#include "clueapi/http/multipart/detail/types/cfg/cfg.hxx"

#include "clueapi/shared/macros.hxx"

namespace clueapi::http::multipart::detail {
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