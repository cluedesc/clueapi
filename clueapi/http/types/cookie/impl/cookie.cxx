/**
 * @file cookie.cxx
 *
 * @brief Implements the cookie type.
 */

#include "clueapi/http/types/cookie/cookie.hxx"

#include "clueapi/exceptions/exceptions.hxx"

#include "clueapi/http/types/cookie/detail/detail.hxx"

#include <fmt/base.h>
#include <fmt/chrono.h>
#include <fmt/compile.h>

namespace clueapi::http::types {
    exceptions::expected_t<raw_cookie_t> cookie_t::serialize(const cookie_t& cookie) noexcept {
        if (!cookie.m_secure && cookie.m_name.starts_with("__Secure-")) {
            return exceptions::make_unexpected(
                exceptions::invalid_argument_t::make("__Secure- cookies require secure flag"));
        }

        if (cookie.m_name.starts_with("__Host-")) {
            if (!cookie.m_secure || !cookie.m_domain.empty() || cookie.m_path != "/") {
                return exceptions::make_unexpected(exceptions::invalid_argument_t::make(
                    "__Host- cookies require secure, no domain, Path=/"));
            }
        }

        fmt::basic_memory_buffer<char, detail::k_cookie_buf_capacity> buf{};

        {
            const auto reserve =
                cookie.m_name.size() + cookie.m_value.size() + detail::k_cookie_buf_reserve;

            if (reserve > detail::k_cookie_buf_capacity)
                buf.reserve(reserve);
        }

        fmt::format_to(
            std::back_inserter(buf), FMT_COMPILE("{}={}"), cookie.m_name, cookie.m_value);

        if (!cookie.m_domain.empty())
            fmt::format_to(std::back_inserter(buf), FMT_COMPILE("; Domain={}"), cookie.m_domain);

        if (!cookie.m_path.empty())
            fmt::format_to(std::back_inserter(buf), FMT_COMPILE("; Path={}"), cookie.m_path);

        if (cookie.m_max_age.count() > 0) {
            fmt::format_to(
                std::back_inserter(buf), FMT_COMPILE("; Max-Age={}"), cookie.m_max_age.count());

            const auto expires_time = std::chrono::system_clock::now() + cookie.m_max_age;

            fmt::format_to(
                std::back_inserter(buf),
                FMT_COMPILE("; Expires={:%a, %d %b %Y %H:%M:%S} GMT"),
                expires_time);
        }

        if (cookie.m_secure)
            fmt::format_to(std::back_inserter(buf), FMT_COMPILE("; Secure"));

        if (cookie.m_http_only)
            fmt::format_to(std::back_inserter(buf), FMT_COMPILE("; HttpOnly"));

        if (!cookie.m_same_site.empty()) {
            fmt::format_to(
                std::back_inserter(buf), FMT_COMPILE("; SameSite={}"), cookie.m_same_site);
        }

        return exceptions::expected_t<raw_cookie_t>{std::string{buf.data(), buf.size()}};
    }

    exceptions::expected_t<raw_cookie_t> cookie_t::serialize() noexcept {
        return serialize(*this);
    }
} // namespace clueapi::http::types