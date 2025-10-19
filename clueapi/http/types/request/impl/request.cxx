/**
 * @file request.cxx
 *
 * @brief Implements the request type.
 */

#include "clueapi/http/types/request/request.hxx"

#include <algorithm>

namespace clueapi::http::types {
    void request_t::parse_cookies() const noexcept {
        if (m_cookies_parsed)
            return;

        m_cookies_parsed = true;

        auto cookie_header = m_headers.find("cookie");

        if (cookie_header == m_headers.end())
            return;

        std::string_view cookie_value = cookie_header->second;

        if (m_cookies.empty()) {
            m_cookies_buffer = std::string{cookie_value};

            std::string_view cookies_view = m_cookies_buffer;

            std::size_t pos{};

            while (pos < cookies_view.length()) {
                auto next_semi = cookies_view.find(';', pos);

                if (next_semi == std::string_view::npos)
                    next_semi = cookies_view.length();

                auto pair = cookies_view.substr(pos, next_semi - pos);

                pos = next_semi + 1;

                pair.remove_prefix(std::min(pair.find_first_not_of(" \t"), pair.size()));

                auto eq_pos = pair.find('=');

                if (eq_pos != std::string_view::npos) {
                    auto key = pair.substr(0, eq_pos);
                    auto val = pair.substr(eq_pos + 1);

                    while (!key.empty() && std::isspace(static_cast<unsigned char>(key.back())))
                        key.remove_suffix(1);

                    val.remove_prefix(std::min(val.find_first_not_of(" \t"), val.size()));

                    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back())))
                        val.remove_suffix(1);

                    m_cookies.emplace(key, val);
                }
            }
        }
    }
} // namespace clueapi::http::types