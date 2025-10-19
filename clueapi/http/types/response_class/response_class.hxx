/**
 * @file response_class.hxx
 * 
 * @brief Defines the response class enum.
 */

#ifndef CLUEAPI_HTTP_TYPES_RESPONSE_CLASS_HXX
#define CLUEAPI_HTTP_TYPES_RESPONSE_CLASS_HXX

namespace clueapi::http::types {
    /**
     * @enum e_response_class
     *
     * @brief Defines the default class for error responses (e.g., plain text or JSON).
     */
    enum struct e_response_class : std::uint8_t { plain, json };
}

#endif // CLUEAPI_HTTP_TYPES_RESPONSE_CLASS_HXX