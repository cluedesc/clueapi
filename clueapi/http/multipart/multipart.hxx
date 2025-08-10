/**
 * @file multipart.hxx
 *
 * @brief Defines the parser for `multipart/form-data` content.
 */

#ifndef CLUEAPI_HTTP_MULTIPART_HXX
#define CLUEAPI_HTTP_MULTIPART_HXX

/**
 * @namespace clueapi::http::multipart
 *
 * @brief The main namespace for the clueapi HTTP multipart parser.
 */
namespace clueapi::http::multipart {
    /**
     * @struct parser_t
     *
     * @brief A parser for `multipart/form-data` bodies, handling both in-memory and file-based streaming.
     *
     * @details This class is responsible for parsing raw HTTP bodies that are encoded as `multipart/form-data`.
     * It can handle data directly from a string view or stream data from a file on disk to efficiently
     * process large file uploads without consuming excessive memory.
     */
    struct parser_t {
        /**
         * @struct cfg_t
         *
         * @brief Configuration settings for the multipart parser.
         */
        struct cfg_t {
            /**
             * @brief The boundary string used to separate parts of the multipart message.
             *
             * @note This is typically extracted from the Content-Type header and set internally.
             */
            std::string_view m_boundary{};

            /**
             * @brief The size of the buffer chunk used when reading from a file stream.
             */
            std::size_t m_chunk_size{65536}; // 64 KiB

            /**
             * @brief The maximum size (in bytes) a single uploaded file can have before
             * it is written to a temporary file on disk.
             */
            std::size_t m_max_file_size_in_memory{1048576}; // 1 MiB

            /**
             * @brief The cumulative maximum size (in bytes) of all uploaded files that can be stored in memory.
             *
             * @details If this limit is exceeded, subsequent file uploads will be streamed to temporary files.
             */
            std::size_t m_max_files_size_in_memory{1048576 * 10}; // 10 MiB

            /**
             * @brief The maximum number of parts (files + fields) allowed in a single request to prevent abuse.
             */
            std::size_t m_max_parts_count{1024};
        };

        /**
         * @struct headers_t
         *
         * @brief Holds the parsed headers for a single part of a multipart message.
         */
        struct headers_t {
            /**
             * @brief The content type of the part.
             */
            std::string m_content_type{};

            /**
             * @brief The file name of the part.
             */
            std::string m_file_name{};

            /**
             * @brief The name of the part.
             */
            std::string m_name{};
        };

        /**
         * @struct parsed_parts_t
         *
         * @brief Contains the results of a successful parse operation.
         */
        struct parsed_parts_t {
            /**
             * @brief The uploaded files from the multipart request.
             */
            types::files_t m_files{};

            /**
             * @brief The form fields from the multipart request.
             */
            types::fields_t m_fields{};
        };

      public:
        /**
         * @brief Constructs a multipart parser with the given configuration.
         *
         * @param cfg The configuration settings for this parser instance.
         */
        CLUEAPI_INLINE parser_t(cfg_t cfg) : m_cfg(std::move(cfg)) {
            m_dash_boundary = fmt::format(FMT_COMPILE("--{}"), m_cfg.m_boundary);

            m_crlf_dash_boundary = fmt::format(FMT_COMPILE("\r\n{}"), m_dash_boundary);
        }

      public:
        /**
         * @brief Asynchronously parses a multipart body from an in-memory string view.
         *
         * @param body The `std::string_view` containing the entire multipart body.
         *
         * @return An `expected_awaitable_t` resolving to a `parsed_parts_t` on success, or an error.
         *
         * @details This method is suitable for smaller request bodies that can be held entirely in memory.
         * For larger uploads, it will transparently write files to disk if they exceed configured memory limits.
         */
        CLUEAPI_NOINLINE exceptions::expected_awaitable_t<parsed_parts_t> parse(std::string_view body) {
            parsed_parts_t ret{};

            std::size_t total_in_memory_size{};

            if (!body.starts_with(m_dash_boundary)) {
                co_return exceptions::make_unexpected(
                    exceptions::runtime_error_t::make("Body does not start with boundary")
                );
            }

            body.remove_prefix(m_dash_boundary.length());

            while (true) {
                if (ret.m_fields.size() + ret.m_files.size() >= m_cfg.m_max_parts_count) {
                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Maximum number of parts exceeded")
                    );
                }

                if (body.starts_with("--"))
                    co_return exceptions::expected_t<parsed_parts_t>{std::move(ret)};

                if (!body.starts_with("\r\n"))
                    co_return exceptions::make_unexpected(exceptions::runtime_error_t::make("Malformed boundary line"));

                body.remove_prefix(2);

                const auto headers_end_pos = body.find("\r\n\r\n");

                if (headers_end_pos == std::string_view::npos) {
                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Can't find headers end section")
                    );
                }

                auto headers_blob = body.substr(0, headers_end_pos);

                auto headers = parse_headers(headers_blob);

                body.remove_prefix(headers_end_pos + 4);

                auto content_end_pos = body.find(m_crlf_dash_boundary);

                if (content_end_pos == std::string_view::npos) {
                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Can't find content end section")
                    );
                }

                const auto content = body.substr(0, content_end_pos);

                if (!headers.m_name.empty()) {
                    const auto content_len = content.length();

                    const auto is_file = !headers.m_file_name.empty();

                    if (is_file) {
                        if (content_len > m_cfg.m_max_file_size_in_memory
                            || total_in_memory_size + content_len > m_cfg.m_max_files_size_in_memory) {
                            boost::system::error_code ec{};

                            auto executor = co_await boost::asio::this_coro::executor;

                            auto temp_path = boost::filesystem::temp_directory_path()
                                           / boost::filesystem::unique_path("clueapi-%%%%-%%%%.tmp");

                            boost::asio::stream_file temp_file(executor);

                            temp_file.open(
                                temp_path.string(),

                                boost::asio::stream_file::write_only | boost::asio::stream_file::create, ec
                            );

                            if (ec) {
                                co_return exceptions::make_unexpected(
                                    exceptions::io_error_t::make("Failed to create temp file: {}", ec.message())
                                );
                            }

                            co_await boost::asio::async_write(
                                temp_file,

                                boost::asio::buffer(content),

                                boost::asio::use_awaitable
                            );

                            temp_file.close();

                            ret.m_files.emplace(
                                headers.m_name,

                                types::file_t{
                                    std::move(headers.m_file_name), std::move(headers.m_content_type),
                                    std::move(temp_path)
                                }
                            );
                        }
                        else {
                            total_in_memory_size += content_len;

                            if (content_len > m_cfg.m_chunk_size) {
                                auto executor = co_await boost::asio::this_coro::executor;

                                co_await boost::asio::post(executor, boost::asio::use_awaitable);
                            }

                            std::vector<char> data(content.begin(), content.end());

                            ret.m_files.emplace(
                                headers.m_name,

                                types::file_t{
                                    std::move(headers.m_file_name), std::move(headers.m_content_type), std::move(data)
                                }
                            );
                        }
                    }
                    else {
                        ret.m_fields.emplace(
                            headers.m_name,

                            std::string(content.begin(), content.end())
                        );
                    }
                }

                body.remove_prefix(content_end_pos + m_crlf_dash_boundary.length());
            }
        }

        /**
         * @brief Asynchronously parses a multipart body by streaming it from a file.
         *
         * @param file_path The path to the file containing the multipart body.
         *
         * @return An `expected_awaitable_t` resolving to a `parsed_parts_t` on success, or an unexpected.
         *
         * @details This method is designed for handling large request bodies that have already been
         * streamed to a temporary file on disk. It reads the file in chunks to keep memory usage low.
         */
        CLUEAPI_NOINLINE exceptions::expected_awaitable_t<parsed_parts_t>
                         parse_file(const boost::filesystem::path& file_path) {
            parsed_parts_t ret{};

            boost::system::error_code ec{};

            auto executor = co_await boost::asio::this_coro::executor;

            boost::asio::stream_file file(executor);

            file.open(file_path.string(), boost::asio::stream_file::read_only, ec);

            if (ec) {
                file.close();

                co_return exceptions::make_unexpected(
                    exceptions::io_error_t::make("Failed to open file: {}", ec.message())
                );
            }

            std::string_view buffer_view{};

            std::string processing_buffer{};

            processing_buffer.reserve(m_cfg.m_chunk_size);

            bool eof_reached{};

            auto read_more_data = [&, this](std::string_view& current_view) -> shared::awaitable_t<bool> {
                if (eof_reached)
                    co_return false;

                if (!current_view.empty()) {
                    const auto processed_prefix_len = current_view.data() - processing_buffer.data();

                    if (processed_prefix_len > 0)
                        processing_buffer.erase(0, processed_prefix_len);
                }

                const auto tail_size = current_view.length();

                processing_buffer.resize(tail_size + m_cfg.m_chunk_size);

                auto bytes_read = co_await file.async_read_some(
                    boost::asio::buffer(processing_buffer.data() + tail_size, m_cfg.m_chunk_size),

                    boost::asio::redirect_error(boost::asio::use_awaitable, ec)
                );

                processing_buffer.resize(tail_size + bytes_read);

                if (ec == boost::asio::error::eof) {
                    eof_reached = true;

                    ec.clear();
                }
                else if (ec) {
                    file.close();

                    co_return false;
                }

                current_view = std::string_view(processing_buffer);

                co_return true;
            };

            bool found_first_boundary{};

            while (!found_first_boundary) {
                auto pos = buffer_view.find(m_dash_boundary);

                if (pos != std::string_view::npos) {
                    buffer_view.remove_prefix(pos + m_dash_boundary.length());

                    found_first_boundary = true;

                    break;
                }

                const auto keep_tail_len = std::max(m_crlf_dash_boundary.length(), m_dash_boundary.length()) - 1;

                if (buffer_view.length() > keep_tail_len)
                    buffer_view.remove_prefix(buffer_view.length() - keep_tail_len);

                if (eof_reached && !found_first_boundary) {
                    file.close();

                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("First boundary not found in file")
                    );
                }

                if (!co_await read_more_data(buffer_view)) {
                    file.close();

                    co_return exceptions::make_unexpected(
                        exceptions::io_error_t::make("Failed to read file: {}", ec.message())
                    );
                }
            }

            while (true) {
                if (ret.m_fields.size() + ret.m_files.size() >= m_cfg.m_max_parts_count) {
                    file.close();

                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Maximum number of parts exceeded")
                    );
                }

                if (buffer_view.starts_with("--") || buffer_view.starts_with("\r\n--")) {
                    file.close();

                    co_return exceptions::expected_t<parsed_parts_t>{std::move(ret)};
                }

                if (buffer_view.starts_with("\r\n"))
                    buffer_view.remove_prefix(2);

                headers_t headers{};

                bool headers_found{};

                while (!headers_found) {
                    auto pos = buffer_view.find("\r\n\r\n");

                    if (pos != std::string_view::npos) {
                        headers = parse_headers(buffer_view.substr(0, pos));

                        buffer_view.remove_prefix(pos + 4);

                        headers_found = true;

                        break;
                    }

                    if (eof_reached) {
                        file.close();

                        co_return exceptions::make_unexpected(
                            exceptions::io_error_t::make("Headers section not found, unexpected EOF")
                        );
                    }

                    if (buffer_view.length() > 1024) {
                        file.close();

                        co_return exceptions::make_unexpected(
                            exceptions::runtime_error_t::make("Headers too large or malformed")
                        );
                    }

                    if (!co_await read_more_data(buffer_view)) {
                        file.close();

                        co_return exceptions::make_unexpected(
                            exceptions::runtime_error_t::make("Failed to read headers: {}", ec.message())
                        );
                    }
                }

                if (headers.m_name.empty())
                    continue;

                const auto is_file = !headers.m_file_name.empty();

                if (is_file) {
                    std::variant<std::vector<char>, temp_file_storage_t> part_storage = std::vector<char>();

                    auto in_memory = true;

                    std::size_t part_size{};

                    bool part_complete{};

                    while (!part_complete) {
                        auto pos = buffer_view.find(m_crlf_dash_boundary);

                        if (pos != std::string_view::npos) {
                            const auto final_chunk = buffer_view.substr(0, pos);

                            part_size += final_chunk.length();

                            if (in_memory) {
                                auto& vec = std::get<std::vector<char>>(part_storage);

                                if (part_size > m_cfg.m_max_file_size_in_memory) {
                                    auto temp_path = boost::filesystem::temp_directory_path()
                                                   / boost::filesystem::unique_path("clueapi-%%%%-%%%%.tmp");

                                    temp_file_storage_t new_storage(executor, temp_path);

                                    new_storage.m_file.open(
                                        temp_path.string(),
                                        boost::asio::stream_file::write_only | boost::asio::stream_file::create, ec
                                    );

                                    if (ec) {
                                        co_return exceptions::make_unexpected(
                                            exceptions::io_error_t::make("Failed to create temp file: {}", ec.message())
                                        );
                                    }

                                    if (!vec.empty()) {
                                        co_await boost::asio::async_write(
                                            new_storage.m_file,

                                            boost::asio::buffer(vec),

                                            boost::asio::use_awaitable
                                        );
                                    }

                                    co_await boost::asio::async_write(
                                        new_storage.m_file,

                                        boost::asio::buffer(final_chunk),

                                        boost::asio::use_awaitable
                                    );

                                    part_storage = std::move(new_storage);

                                    in_memory = false;
                                }
                                else
                                    vec.insert(vec.end(), final_chunk.begin(), final_chunk.end());
                            }
                            else {
                                co_await boost::asio::async_write(
                                    std::get<temp_file_storage_t>(part_storage).m_file,

                                    boost::asio::buffer(final_chunk),

                                    boost::asio::use_awaitable

                                );
                            }

                            buffer_view.remove_prefix(pos + m_crlf_dash_boundary.length());

                            part_complete = true;

                            break;
                        }

                        std::string_view chunk_to_write{};

                        if (buffer_view.length() > m_crlf_dash_boundary.length()) {
                            const auto safe_len = buffer_view.length() - m_crlf_dash_boundary.length();

                            chunk_to_write = buffer_view.substr(0, safe_len);
                        }

                        if (!chunk_to_write.empty()) {
                            part_size += chunk_to_write.length();

                            if (in_memory && part_size > m_cfg.m_max_file_size_in_memory) {
                                auto temp_path = boost::filesystem::temp_directory_path()
                                               / boost::filesystem::unique_path("clueapi-%%%%-%%%%.tmp");

                                temp_file_storage_t new_storage(executor, temp_path);

                                new_storage.m_file.open(
                                    temp_path.string(),
                                    boost::asio::stream_file::write_only | boost::asio::stream_file::create, ec
                                );

                                if (ec) {
                                    co_return exceptions::make_unexpected(
                                        exceptions::io_error_t::make("Failed to create temp file: {}", ec.message())
                                    );
                                }

                                auto& vec = std::get<std::vector<char>>(part_storage);

                                co_await boost::asio::async_write(
                                    new_storage.m_file,

                                    boost::asio::buffer(vec),

                                    boost::asio::use_awaitable
                                );

                                part_storage = std::move(new_storage);

                                in_memory = false;
                            }

                            if (in_memory) {
                                auto& vec = std::get<std::vector<char>>(part_storage);

                                vec.insert(vec.end(), chunk_to_write.begin(), chunk_to_write.end());
                            }
                            else {
                                co_await boost::asio::async_write(
                                    std::get<temp_file_storage_t>(part_storage).m_file,

                                    boost::asio::buffer(chunk_to_write),

                                    boost::asio::use_awaitable
                                );
                            }

                            buffer_view.remove_prefix(chunk_to_write.length());
                        }

                        if (eof_reached && !part_complete) {
                            file.close();

                            co_return exceptions::make_unexpected(
                                exceptions::runtime_error_t::make("Part boundary not found, unexpected EOF")
                            );
                        }

                        if (!eof_reached && buffer_view.length() <= m_crlf_dash_boundary.length()) {
                            if (!co_await read_more_data(buffer_view)) {
                                file.close();

                                co_return exceptions::make_unexpected(
                                    exceptions::io_error_t::make("Failed to read file part: {}", ec.message())
                                );
                            }
                        }
                    }

                    if (in_memory) {
                        ret.m_files.emplace(
                            headers.m_name,

                            types::file_t{
                                std::move(headers.m_file_name), std::move(headers.m_content_type),
                                std::move(std::get<std::vector<char>>(part_storage))
                            }
                        );
                    }
                    else {
                        auto& storage = std::get<temp_file_storage_t>(part_storage);

                        storage.m_file.close(ec);

                        ret.m_files.emplace(
                            headers.m_name,
                            types::file_t{
                                std::move(headers.m_file_name), std::move(headers.m_content_type),
                                std::move(storage.m_path)
                            }
                        );
                    }
                }
                else {
                    std::string field_value{};

                    bool field_complete{};

                    while (!field_complete) {
                        auto pos = buffer_view.find(m_crlf_dash_boundary);

                        if (pos != std::string_view::npos) {
                            field_value.append(buffer_view.substr(0, pos));

                            buffer_view.remove_prefix(pos + m_crlf_dash_boundary.length());

                            field_complete = true;

                            break;
                        }

                        std::string_view chunk_to_write{};

                        if (buffer_view.length() > m_crlf_dash_boundary.length()) {
                            const auto safe_len = buffer_view.length() - m_crlf_dash_boundary.length();

                            chunk_to_write = buffer_view.substr(0, safe_len);
                        }

                        if (!chunk_to_write.empty()) {
                            field_value.append(chunk_to_write);

                            buffer_view.remove_prefix(chunk_to_write.length());
                        }

                        if (eof_reached && !field_complete) {
                            file.close();

                            co_return exceptions::make_unexpected(
                                exceptions::runtime_error_t::make("Part boundary not found for field, unexpected EOF")
                            );
                        }

                        if (!eof_reached && buffer_view.length() <= m_crlf_dash_boundary.length()) {
                            if (!co_await read_more_data(buffer_view)) {
                                file.close();

                                co_return exceptions::make_unexpected(
                                    exceptions::runtime_error_t::make("Failed to read field: {}", ec.message())
                                );
                            }
                        }
                    }

                    ret.m_fields.emplace(headers.m_name, std::move(field_value));
                }
            }

            file.close();

            co_return exceptions::make_unexpected(exceptions::io_error_t::make("Parsing ended unexpectedly"));
        }

      private:
        /**
         * @brief Splits a string view by a delimiter.
         */
        CLUEAPI_INLINE auto split(
            const std::string_view& str,

            const std::string_view& delimiter
        ) {
            return std::views::split(str, delimiter) | std::views::transform([](auto&& subrange) {
                       return std::string_view(subrange.begin(), subrange.end());
                   });
        }

        /**
         * @brief Parses the headers of a single multipart part.
         *
         * @param headers_blob A string view containing the raw headers for one part.
         *
         * @return A `headers_t` struct populated with the parsed values.
         */
        CLUEAPI_INLINE headers_t parse_headers(const std::string_view headers_blob) {
            headers_t headers{};

            bool prioritize_file_name{};

            bool content_disposition_found{};

            for (std::string_view line : split(headers_blob, "\r\n")) {
                if (boost::ifind_first(line, "content-disposition")) {
                    if (content_disposition_found)
                        return {};

                    content_disposition_found = true;

                    for (std::string_view param_part : split(line, ";")) {
                        auto trimmed_param = boost::algorithm::trim_copy(param_part);

                        if (trimmed_param.empty())
                            continue;

                        auto eq_pos = trimmed_param.find('=');

                        if (eq_pos == std::string_view::npos)
                            continue;

                        auto key   = trimmed_param.substr(0, eq_pos);
                        auto value = trimmed_param.substr(eq_pos + 1);

                        if (value.starts_with('"') && value.ends_with('"')) {
                            value.remove_prefix(1);
                            value.remove_suffix(1);
                        }

                        if (boost::iequals(key, "name")) {
                            headers.m_name = std::string(value);
                        }
                        else if (boost::iequals(key, "filename*")) {
                            auto value_start_pos = value.find('\'');

                            if (value_start_pos != std::string_view::npos) {
                                value_start_pos = value.find('\'', value_start_pos + 1);

                                if (value_start_pos != std::string_view::npos) {
                                    auto encoded_value = value.substr(value_start_pos + 1);

                                    headers.m_file_name = detail::url_decode(encoded_value);

                                    prioritize_file_name = true;
                                }
                            }
                        }
                        else if (boost::iequals(key, "filename") && !prioritize_file_name)
                            headers.m_file_name = std::string(value);
                    }
                }
                else if (boost::ifind_first(line, "content-type")) {
                    const auto pos = line.find(':');

                    if (pos != std::string::npos) {
                        headers.m_content_type = std::string(line.substr(pos + 1));

                        boost::algorithm::trim(headers.m_content_type);
                    }
                }
            }

            return headers;
        }

      private:
        /**
         * @struct temp_file_storage_t
         *
         * @brief Helper struct for managing a temporary file during file-based parsing.
         */
        struct temp_file_storage_t {
            boost::filesystem::path m_path;

            boost::asio::stream_file m_file;

            temp_file_storage_t(boost::asio::any_io_executor ex, boost::filesystem::path p)
                : m_path(std::move(p)), m_file(std::move(ex)) {}
        };

      private:
        /**
         * @brief The configuration for this parser instance.
         */
        cfg_t m_cfg{};

        /**
         * @brief The boundary string prefixed with "--".
         */
        std::string m_dash_boundary{};

        /**
         * @brief The boundary string prefixed with "\r\n--".
         */
        std::string m_crlf_dash_boundary{};
    };
}

#endif // CLUEAPI_HTTP_MULTIPART_HXX