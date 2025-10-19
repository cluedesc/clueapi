/**
 * @file mime.hxx
 *
 * @brief Provides utilities for determining the MIME type of a file based on its extension.
 */

#ifndef CLUEAPI_HTTP_MIME_HXX
#define CLUEAPI_HTTP_MIME_HXX

#include <boost/filesystem/path.hpp>

#include "clueapi/http/types/mime/mime.hxx"

#include "detail/detail.hxx"

/**
 * @namespace clueapi::http::mime
 *
 * @brief The main namespace for the clueapi HTTP MIME type lookups.
 */
namespace clueapi::http::mime {
    /**
     * @struct mime_t
     *
     * @brief A static utility class for MIME type lookups.
     *
     * @details This class provides a way to get the appropriate MIME type string
     * for a given file path by checking its extension against a predefined map.
     */
    struct mime_t {
        /**
         * @brief Retrieves the static, global map of file extensions to MIME types.
         *
         * @return A const reference to the MIME type map.
         */
        static const types::mime_map_t& mime_map() noexcept;

        /**
         * @brief Determines the MIME type for a given file path.
         *
         * @param path The `boost::filesystem::path` of the file.
         *
         * @return The corresponding MIME type as a `std::string_view`. If the extension is not
         * found or the path is invalid, it returns the default "application/octet-stream".
         */
        static types::mime_type_t mime_type(const boost::filesystem::path& path) noexcept;
    };
} // namespace clueapi::http::mime

#endif // CLUEAPI_HTTP_MIME_HXX