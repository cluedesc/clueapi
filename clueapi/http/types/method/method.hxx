/**
 * @file method.hxx
 *
 * @brief Defines the types and utilities for handling HTTP request methods.
 */

#ifndef CLUEAPI_HTTP_TYPES_METHOD_HXX
#define CLUEAPI_HTTP_TYPES_METHOD_HXX

namespace clueapi::http::types {
    /**
     * @struct method_t
     *
     * @brief A utility struct for working with HTTP methods.
     *
     * @details Provides a scoped enum `e_method` for representation of HTTP methods
     * and static constexpr functions for converting between the enum and its string representation.
     */
    struct method_t {
        /**
         * @enum e_method
         *
         * @brief A scoped enumeration of standard HTTP methods.
         */
        enum e_method : std::uint8_t {
            unknown,
            get,
            head,
            post,
            put,
            delete_,
            connect,
            options,
            trace,
            patch,
            count
        };

        /**
         * @brief Converts an `e_method` enum value to its corresponding string representation.
         *
         * @param method The enum value to convert.
         *
         * @return A `std::string_view` of the HTTP method (e.g., "GET", "POST").
         * Returns "UNKNOWN" if the method is not recognized.
         */
        CLUEAPI_INLINE static constexpr std::string_view to_str(e_method method) noexcept {
            return method < k_method_names.size() ? k_method_names[method] : "UNKNOWN";
        }

        /**
         * @brief Converts a string representation of an HTTP method to its corresponding `e_method`
         * enum value.
         *
         * @param str The string view of the method to convert (e.g., "GET").
         *
         * @return The corresponding `e_method` enum value.
         * Returns `e_method::unknown` if the string is not a valid method.
         *
         * @details This function uses a switch on string length as a performance optimization for
         * faster lookups.
         */
        CLUEAPI_INLINE static constexpr e_method from_str(std::string_view str) noexcept {
            // note: performance trick
            switch (str.size()) {
                case 3:
                    if (str == "GET")
                        return e_method::get;

                    if (str == "PUT")
                        return e_method::put;

                    break;
                case 4:
                    if (str == "HEAD")
                        return e_method::head;

                    if (str == "POST")
                        return e_method::post;

                    break;
                case 5:
                    if (str == "PATCH")
                        return e_method::patch;

                    if (str == "TRACE")
                        return e_method::trace;

                    break;
                case 6:
                    if (str == "DELETE")
                        return e_method::delete_;

                    break;
                case 7:
                    if (str == "CONNECT")
                        return e_method::connect;

                    if (str == "OPTIONS")
                        return e_method::options;

                    break;
                default:
                    break;
            }

            return e_method::unknown;
        }

       private:
        /**
         * @brief A compile-time array holding the string names of HTTP methods, indexed by
         * `e_method`.
         */
        static constexpr std::array<std::string_view, e_method::count> k_method_names = {
            "UNKNOWN",
            "GET",
            "HEAD",
            "POST",
            "PUT",
            "DELETE",
            "CONNECT",
            "OPTIONS",
            "TRACE",
            "PATCH"};
    };
} // namespace clueapi::http::types

#endif // CLUEAPI_HTTP_TYPES_METHOD_HXX