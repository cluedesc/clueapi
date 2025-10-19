/**
 * @file cfg.hxx
 * 
 * @brief Defines the configuration struct for the multipart parser.
 */

#ifndef CLUEAPI_HTTP_MULTIPART_DETAIL_TYPES_CFG_HXX
#define CLUEAPI_HTTP_MULTIPART_DETAIL_TYPES_CFG_HXX

#include <string_view>

#include <cstddef>

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
        std::size_t m_max_files_size_in_memory{1048576ull * 10u};

        /**
         * @brief The maximum number of parts (files + fields) allowed in a single request to
         * prevent abuse.
         */
        std::size_t m_max_parts_count{1024};
    };
} // namespace clueapi::http::multipart::detail

#endif // CLUEAPI_HTTP_MULTIPART_DETAIL_TYPES_CFG_HXX