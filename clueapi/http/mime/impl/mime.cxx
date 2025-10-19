/**
 * @file mime.cxx
 *
 * @brief Implements the MIME type lookup.
 */

#include "clueapi/http/mime/mime.hxx"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <boost/filesystem/path.hpp>

namespace clueapi::http::mime {
    const types::mime_map_t& mime_t::mime_map() noexcept {
        static const types::mime_map_t mime_map = [] {
            types::mime_map_t ret{};

            ret.reserve(detail::k_mime_entries.size());

            for (const auto& [ext, mime] : detail::k_mime_entries) {
                if (ext.empty() || mime.empty())
                    continue;

                ret.emplace(ext, mime);
            }

            return ret;
        }();

        return mime_map;
    }

    types::mime_type_t mime_t::mime_type(const boost::filesystem::path& path) noexcept {
        if (path.empty())
            return detail::k_def_mime_type;

        const auto ext = path.extension();

        if (ext.empty())
            return detail::k_def_mime_type;

        const auto lower_ext = boost::algorithm::to_lower_copy(ext.string());

        for (const auto& [e, m] : detail::k_mime_entries)
            if (boost::algorithm::iequals(lower_ext, e))
                return m;

        return detail::k_def_mime_type;
    }
} // namespace clueapi::http::mime