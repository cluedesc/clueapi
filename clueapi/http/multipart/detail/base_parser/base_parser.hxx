/**
 * @file base_parser.hxx
 *
 * @brief A file for defining the details of the multipart parser.
 */

#ifndef CLUEAPI_HTTP_MULTIPART_DETAIL_BASE_PARSER_HXX
#define CLUEAPI_HTTP_MULTIPART_DETAIL_BASE_PARSER_HXX

#include "clueapi/exceptions/wrap/wrap.hxx"

#include "clueapi/http/multipart/detail/types/types.hxx"

/**
 * @namespace clueapi::http::multipart::detail
 *
 * @brief Defines the details of the multipart parser.
 */
namespace clueapi::http::multipart::detail {
    /**
     * @brief The base class for the multipart parser.
     */
    struct base_parser_t {
        virtual ~base_parser_t() noexcept = default;

        /**
         * @brief Virtual function to parse the multipart body.
         *
         * @return
         */
        virtual exceptions::expected_awaitable_t<parts_t> parse() = 0;
    };
} // namespace clueapi::http::multipart::detail

#endif // CLUEAPI_HTTP_MULTIPART_DETAIL_BASE_PARSER_HXX