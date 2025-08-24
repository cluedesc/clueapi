/**
 * @file headers_parser.hxx
 *
 * @brief A file for defining the headers parser.
 */

#ifndef CLUEAPI_HTTP_MULTIPART_DETAIL_HEADERS_PARSER_HXX
#define CLUEAPI_HTTP_MULTIPART_DETAIL_HEADERS_PARSER_HXX

namespace clueapi::http::multipart::detail {
    /**
     * @struct headers_t
     *
     * @brief Holds the parsed headers for a single part of a multipart message.
     */
    struct headers_t {
        CLUEAPI_INLINE constexpr headers_t() noexcept = default;

       public:
        /**
         * @brief The content type.
         */
        std::string m_content_type;

        /**
         * @brief The file name.
         */
        std::string m_file_name;

        /**
         * @brief The name.
         */
        std::string m_name;
    };

    /**
     * @brief Parses header parameters (e.g., from Content-Disposition).
     *
     * @param params_sv The string_view containing parameters.
     *
     * @return A vector of key-value pairs.
     */
    CLUEAPI_INLINE std::vector<std::pair<std::string_view, std::string_view>> parse_parameters(
        std::string_view params_sv) noexcept {
        std::vector<std::pair<std::string_view, std::string_view>> params;

        const auto* head = params_sv.begin();

        const auto* const end = params_sv.end();

        while (head != end) {
            while (head != end && (*head == ';' || *head == ' '))
                head++;

            if (head == end)
                break;

            const auto* const eq_it = std::find(head, end, '=');

            if (eq_it == end)
                break;

            auto key =
                shared::non_copy::trim({head, static_cast<size_t>(std::distance(head, eq_it))});

            head = eq_it + 1;

            std::string_view value{};

            if (head != end && *head == '"') {
                head++;

                const auto* value_start = head;

                while (head != end) {
                    if (*head == '"')
                        break;

                    if (*head == '\\' && (head + 1) != end)
                        head++;

                    head++;
                }

                value = {value_start, static_cast<size_t>(std::distance(value_start, head))};

                if (head != end)
                    head++;
            } else {
                const auto* value_start = head;

                const auto* const semi_it = std::find(head, end, ';');

                value = shared::non_copy::trim(
                    {value_start, static_cast<size_t>(std::distance(value_start, semi_it))});

                head = semi_it;
            }

            if (!key.empty())
                params.emplace_back(key, value);
        }

        return params;
    }

    /**
     * @brief Unfolds folded HTTP headers into single lines.
     *
     * @param headers_blob The raw headers block.
     *
     * @return A string with unfolded headers.
     */
    CLUEAPI_INLINE std::string unfold_headers(std::string_view headers_blob) noexcept {
        std::string unfolded{headers_blob};

        boost::replace_all(unfolded, "\r\n ", " ");
        boost::replace_all(unfolded, "\r\n\t", " ");

        return unfolded;
    }

    /**
     * @brief Parses the headers of a single multipart part in a robust manner.
     *
     * @param raw_headers_blob A string view containing the raw headers for one part.
     *
     * @return A `headers_t` struct populated with the parsed values.
     */
    CLUEAPI_INLINE headers_t parse_headers(std::string_view raw_headers_blob) noexcept {
        headers_t headers{};

        auto unfolded_blob = unfold_headers(raw_headers_blob);

        bool filename_star_found{};

        for (auto line : shared::non_copy::split(unfolded_blob, "\r\n")) {
            auto colon_pos = line.find(':');

            if (colon_pos == std::string_view::npos)
                continue;

            auto header_name = line.substr(0, colon_pos);

            auto header_value = shared::non_copy::trim(line.substr(colon_pos + 1));

            if (boost::iequals(header_name, "Content-Type")) {
                headers.m_content_type = std::string{header_value};
            } else if (boost::iequals(header_name, "Content-Disposition")) {
                auto params_start = header_value.find(';');

                if (params_start == std::string_view::npos)
                    continue;

                auto params_sv = header_value.substr(params_start + 1);

                auto parsed_params = parse_parameters(params_sv);

                for (const auto& [key, value] : parsed_params) {
                    if (boost::iequals(key, "name")) {
                        headers.m_name = std::string{shared::non_copy::unquote(value)};
                    } else if (boost::iequals(key, "filename*")) {
                        auto first_quote_pos = value.find('\'');

                        auto second_quote_pos = (first_quote_pos != std::string_view::npos)
                                                    ? value.find('\'', first_quote_pos + 1)
                                                    : std::string_view::npos;

                        if (second_quote_pos != std::string_view::npos) {
                            auto encoded_value = value.substr(second_quote_pos + 1);

                            headers.m_file_name = http::detail::url_decode(encoded_value);

                            filename_star_found = true;
                        }
                    } else if (boost::iequals(key, "filename") && !filename_star_found)
                        headers.m_file_name = std::string{shared::non_copy::unquote(value)};
                }
            }
        }

        return headers;
    }
} // namespace clueapi::http::multipart::detail

#endif // CLUEAPI_HTTP_MULTIPART_DETAIL_HEADERS_PARSER_HXX