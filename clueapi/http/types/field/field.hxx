/**
 * @file field.hxx
 *
 * @brief Defines the type for storing form fields from a multipart request.
 */

#ifndef CLUEAPI_HTTP_TYPES_FIELD_HXX
#define CLUEAPI_HTTP_TYPES_FIELD_HXX

#include "clueapi/http/detail/sv_hash/sv_hash.hxx"

#include "clueapi/shared/shared.hxx"

namespace clueapi::http::types {
    /**
     * @brief Type alias for a map of form fields from a `multipart/form-data` request.
     *
     * @details The key is the field name, and the value is the field's content.
     * It uses a fast, dense hash map for efficient lookups.
     */
    using fields_t = shared::unordered_map_t<
        std::string,
        std::string,

        http::detail::sv_hash_t,
        http::detail::sv_eq_t>;
} // namespace clueapi::http::types

#endif // CLUEAPI_HTTP_TYPES_FIELD_HXX