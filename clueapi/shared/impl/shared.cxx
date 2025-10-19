/**
 * @file shared.cxx
 *
 * @brief Implements shared utilities.
 */

#include "clueapi/shared/shared.hxx"

#include <iterator>

namespace clueapi::shared {
    CLUEAPI_INLINE std::string sanitize_filename(std::string_view original_name) {
        if (original_name.empty())
            return "untitled";

        std::string sanitized{};

        sanitized.reserve(original_name.length());

        std::copy_if(
            original_name.begin(), original_name.end(), std::back_inserter(sanitized), [](char c) {
                if (std::isalnum(static_cast<unsigned char>(c)))
                    return true;

                switch (c) {
                    case '_':
                    case '-':
                    case '.':
                        return true;

                    default:
                        return false;
                }
            });

        if (sanitized.empty() || sanitized == "." || sanitized == "..")
            return "untitled";

        return sanitized;
    }
} // namespace clueapi::shared