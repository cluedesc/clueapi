/**
 * @file detail.hxx
 *
 * @brief Internal data definitions for the MIME type module.
 *
 * @note This header is not intended for direct use by end-users.
 */

#ifndef CLUEAPI_HTTP_MIME_DETAIL_HXX
#define CLUEAPI_HTTP_MIME_DETAIL_HXX

/**
 * @namespace clueapi::http::mime::detail
 *
 * @brief Internal data definitions for the clueapi HTTP MIME type lookups.
 *
 * @internal
 */
namespace clueapi::http::mime::detail {
    /**
     * @brief The default MIME type to return when a lookup fails.
     */
    inline constexpr types::mime_type_t k_def_mime_type = "application/octet-stream";

    /**
     * @brief A compile-time array of common file extensions and their corresponding MIME types.
     *
     * @details This static data is used to populate the runtime MIME map.
     */
    inline constexpr std::array<std::pair<std::string_view, types::mime_type_t>, 41> k_mime_entries{
        {{".html", "text/html"},
         {".htm", "text/html"},
         {".css", "text/css"},
         {".js", "application/javascript"},
         {".json", "application/json"},
         {".xml", "application/xml"},
         {".txt", "text/plain"},
         {".csv", "text/csv"},
         {".md", "text/markdown"},
         {".markdown", "text/markdown"},

         {".png", "image/png"},
         {".jpg", "image/jpeg"},
         {".jpeg", "image/jpeg"},
         {".gif", "image/gif"},
         {".svg", "image/svg+xml"},
         {".ico", "image/vnd.microsoft.icon"},
         {".bmp", "image/bmp"},
         {".webp", "image/webp"},
         {".tif", "image/tiff"},
         {".tiff", "image/tiff"},

         {".mp3", "audio/mpeg"},
         {".ogg", "audio/ogg"},
         {".oga", "audio/ogg"},
         {".opus", "audio/opus"},
         {".wav", "audio/wav"},
         {".mp4", "video/mp4"},
         {".m4a", "audio/mp4"},
         {".m4v", "video/mp4"},
         {".webm", "video/webm"},
         {".ogv", "video/ogg"},

         {".pdf", "application/pdf"},
         {".epub", "application/epub+zip"},
         {".rtf", "application/rtf"},
         {".zip", "application/zip"},
         {".gz", "application/gzip"},
         {".tar", "application/x-tar"},

         {".woff", "font/woff"},
         {".woff2", "font/woff2"},
         {".ttf", "font/ttf"},
         {".otf", "font/otf"},
         {".eot", "application/vnd.ms-fontobject"}}};
} // namespace clueapi::http::mime::detail

#endif // CLUEAPI_HTTP_MIME_DETAIL_HXX