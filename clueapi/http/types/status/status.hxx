/**
 * @file status.hxx
 *
 * @brief Defines the types and utilities for handling HTTP status codes.
 */

#ifndef CLUEAPI_HTTP_TYPES_STATUS_HXX
#define CLUEAPI_HTTP_TYPES_STATUS_HXX

namespace clueapi::http::types {
    /**
     * @struct status_t
     *
     * @brief A utility struct for working with HTTP status codes.
     *
     * @details Provides a scoped enum `e_status` for all standard HTTP status codes,
     * along with static functions to convert them to their standard reason phrase strings.
     */
    struct status_t {
        /**
         * @enum e_status
         *
         * @brief A scoped enumeration of standard HTTP status codes.
         */
        enum e_status : std::uint16_t {
            continue_status = 100,
            switching_protocols = 101,
            processing = 102,
            early_hints = 103,

            ok = 200,
            created = 201,
            accepted = 202,
            non_authoritative_information = 203,
            no_content = 204,
            reset_content = 205,
            partial_content = 206,
            multi_status = 207,
            already_reported = 208,
            im_used = 226,

            multiple_choices = 300,
            moved_permanently = 301,
            found = 302,
            see_other = 303,
            not_modified = 304,
            use_proxy = 305,
            temporary_redirect = 307,
            permanent_redirect = 308,

            bad_request = 400,
            unauthorized = 401,
            payment_required = 402,
            forbidden = 403,
            not_found = 404,
            method_not_allowed = 405,
            not_acceptable = 406,
            proxy_authentication_required = 407,
            request_timeout = 408,
            conflict = 409,
            gone = 410,
            length_required = 411,
            precondition_failed = 412,
            payload_too_large = 413,
            uri_too_long = 414,
            unsupported_media_type = 415,
            range_not_satisfiable = 416,
            expectation_failed = 417,
            im_a_teapot = 418,
            misdirected_request = 421,
            unprocessable_entity = 422,
            locked = 423,
            failed_dependency = 424,
            too_early = 425,
            upgrade_required = 426,
            precondition_required = 428,
            too_many_requests = 429,
            request_header_fields_too_large = 431,
            unavailable_for_legal_reasons = 451,

            internal_server_error = 500,
            not_implemented = 501,
            bad_gateway = 502,
            service_unavailable = 503,
            gateway_timeout = 504,
            http_version_not_supported = 505,
            variant_also_negotiates = 506,
            insufficient_storage = 507,
            loop_detected = 508,
            not_extended = 510,
            network_authentication_required = 511,

            unknown = 1000
        };

        /**
         * @brief Converts an `e_status` enum value to its corresponding reason phrase.
         *
         * @param status The status code enum to convert.
         *
         * @return A `std::string_view` containing the standard reason phrase (e.g., "Not Found").
         */
        CLUEAPI_INLINE static constexpr std::string_view to_str(e_status status) noexcept {
            switch (status) {
                // 1xx Informational
                case e_status::continue_status:
                    return "Continue";

                case e_status::switching_protocols:
                    return "Switching Protocols";

                case e_status::processing:
                    return "Processing";

                case e_status::early_hints:
                    return "Early Hints";

                // 2xx Success
                case e_status::ok:
                    return "OK";

                case e_status::created:
                    return "Created";

                case e_status::accepted:
                    return "Accepted";

                case e_status::non_authoritative_information:
                    return "Non-Authoritative Information";

                case e_status::no_content:
                    return "No Content";

                case e_status::reset_content:
                    return "Reset Content";

                case e_status::partial_content:
                    return "Partial Content";

                case e_status::multi_status:
                    return "Multi-Status";

                case e_status::already_reported:
                    return "Already Reported";

                case e_status::im_used:
                    return "IM Used";

                // 3xx Redirection
                case e_status::multiple_choices:
                    return "Multiple Choices";

                case e_status::moved_permanently:
                    return "Moved Permanently";

                case e_status::found:
                    return "Found";

                case e_status::see_other:
                    return "See Other";

                case e_status::not_modified:
                    return "Not Modified";

                case e_status::use_proxy:
                    return "Use Proxy";

                case e_status::temporary_redirect:
                    return "Temporary Redirect";

                case e_status::permanent_redirect:
                    return "Permanent Redirect";

                // 4xx Client Error
                case e_status::bad_request:
                    return "Bad Request";

                case e_status::unauthorized:
                    return "Unauthorized";

                case e_status::payment_required:
                    return "Payment Required";

                case e_status::forbidden:
                    return "Forbidden";

                case e_status::not_found:
                    return "Not Found";

                case e_status::method_not_allowed:
                    return "Method Not Allowed";

                case e_status::not_acceptable:
                    return "Not Acceptable";

                case e_status::proxy_authentication_required:
                    return "Proxy Authentication Required";

                case e_status::request_timeout:
                    return "Request Timeout";

                case e_status::conflict:
                    return "Conflict";

                case e_status::gone:
                    return "Gone";

                case e_status::length_required:
                    return "Length Required";

                case e_status::precondition_failed:
                    return "Precondition Failed";

                case e_status::payload_too_large:
                    return "Payload Too Large";

                case e_status::uri_too_long:
                    return "URI Too Long";

                case e_status::unsupported_media_type:
                    return "Unsupported Media Type";

                case e_status::range_not_satisfiable:
                    return "Range Not Satisfiable";

                case e_status::expectation_failed:
                    return "Expectation Failed";

                case e_status::im_a_teapot:
                    return "I'm a teapot";

                case e_status::misdirected_request:
                    return "Misdirected Request";

                case e_status::unprocessable_entity:
                    return "Unprocessable Entity";

                case e_status::locked:
                    return "Locked";

                case e_status::failed_dependency:
                    return "Failed Dependency";

                case e_status::too_early:
                    return "Too Early";

                case e_status::upgrade_required:
                    return "Upgrade Required";

                case e_status::precondition_required:
                    return "Precondition Required";

                case e_status::too_many_requests:
                    return "Too Many Requests";

                case e_status::request_header_fields_too_large:
                    return "Request Header Fields Too Large";

                case e_status::unavailable_for_legal_reasons:
                    return "Unavailable For Legal Reasons";

                // 5xx Server Error
                case e_status::internal_server_error:
                    return "Internal Server Error";

                case e_status::not_implemented:
                    return "Not Implemented";

                case e_status::bad_gateway:
                    return "Bad Gateway";

                case e_status::service_unavailable:
                    return "Service Unavailable";

                case e_status::gateway_timeout:
                    return "Gateway Timeout";

                case e_status::http_version_not_supported:
                    return "HTTP Version Not Supported";

                case e_status::variant_also_negotiates:
                    return "Variant Also Negotiates";

                case e_status::insufficient_storage:
                    return "Insufficient Storage";

                case e_status::loop_detected:
                    return "Loop Detected";

                case e_status::not_extended:
                    return "Not Extended";

                case e_status::network_authentication_required:
                    return "Network Authentication Required";

                // Unknown
                case e_status::unknown:
                default:
                    return "Unknown";
            }
        }

        /**
         * @brief Converts a numeric status code to its corresponding reason phrase.
         *
         * @param status The integer status code to convert.
         *
         * @return A `std::string_view` containing the standard reason phrase.
         */
        CLUEAPI_INLINE static constexpr std::string_view to_str(std::size_t status) noexcept {
            return to_str(static_cast<e_status>(status));
        }

        /**
         * @brief Converts a numeric status code to its corresponding reason phrase.
         *
         * @param status The integer status code to convert.
         *
         * @return A `std::string` containing the standard reason phrase.
         */
        CLUEAPI_INLINE static constexpr std::string to_str_copy(std::size_t status) noexcept {
            return std::string{to_str(status)};
        }
    };
} // namespace clueapi::http::types

#endif // CLUEAPI_HTTP_TYPES_STATUS_HXX