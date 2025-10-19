/**
 * @file file_parser.hxx
 *
 * @brief A file for defining the details of the multipart parser.
 */

#ifndef CLUEAPI_HTTP_MULTIPART_DETAIL_FILE_PARSER_HXX
#define CLUEAPI_HTTP_MULTIPART_DETAIL_FILE_PARSER_HXX

#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <cstring>

#include <boost/asio/redirect_error.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <boost/filesystem.hpp>

#include "clueapi/http/multipart/detail/base_parser/base_parser.hxx"
#include "clueapi/http/multipart/detail/headers_parser/headers_parser.hxx"
#include "clueapi/http/multipart/detail/types/types.hxx"

#include "clueapi/exceptions/exceptions.hxx"

/**
 * @namespace clueapi::http::multipart::detail
 *
 * @brief Defines the details of the multipart file parser.
 */
namespace clueapi::http::multipart::detail {
    /**
     * @struct file_parser_t
     *
     * @brief The file parser class.
     *
     * @details This class is responsible for parsing a file part of a multipart message.
     */
    struct file_parser_t : public base_parser_t {
        /**
         * @brief Constructs a file parser.
         *
         * @param executor A reference to the I/O executor.
         * @param file_path The path to the file to parse.
         * @param cfg The configuration for the multipart parser.
         * @param dash_boundary A string_view containing the dash boundary string.
         * @param crlf_dash_boundary A string_view containing the CRLF dash boundary string.
         */
        CLUEAPI_INLINE file_parser_t(
            const boost::asio::any_io_executor& executor,
            const boost::filesystem::path& file_path,
            cfg_t cfg,
            std::string_view dash_boundary,
            std::string_view crlf_dash_boundary) noexcept
            : m_file{executor, file_path},
              m_file_path{file_path},
              m_cfg{cfg},
              m_dash_boundary{dash_boundary},
              m_crlf_dash_boundary{crlf_dash_boundary} {
        }

       public:
        /**
         * @brief Parses the file part.
         *
         * @return An awaitable that resolves to a `parts_t` object containing the parsed parts.
         */
        CLUEAPI_NOINLINE exceptions::expected_awaitable_t<parts_t> parse() override {
            boost::system::error_code ec{};

            auto& file = m_file.file_obj();

            file.open(m_file_path.string(), boost::asio::stream_file::read_only, ec);

            if (ec) {
                co_return exceptions::make_unexpected(
                    exceptions::io_error_t::make("Failed to open file: {}", ec.message()));
            }

            {
                m_buffer_view = {};

                m_eof_reached = false;

                m_total_in_memory_size = 0;
            }

            if (!co_await find_first_boundary()) {
                co_return exceptions::make_unexpected(
                    exceptions::runtime_error_t::make("First boundary not found in file"));
            }

            std::size_t iteration_count{};

            constexpr std::size_t k_max_iterations = 100000;

            parts_t result{};

            while (iteration_count++ < k_max_iterations) {
                if (result.m_fields.size() + result.m_files.size() >= m_cfg.m_max_parts_count) {
                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Maximum number of parts exceeded"));
                }

                if (m_buffer_view.starts_with("--") || m_buffer_view.starts_with("\r\n--"))
                    co_return exceptions::expected_t<parts_t>{std::move(result)};

                if (m_buffer_view.starts_with("\r\n"))
                    m_buffer_view.remove_prefix(2);

                auto headers_result = co_await parse_part_headers();

                if (!headers_result)
                    co_return exceptions::make_unexpected(std::move(headers_result.error()));

                const auto& headers = headers_result.value();

                if (headers.m_name.empty())
                    continue;

                const auto is_file = !headers.m_file_name.empty();

                if (is_file) {
                    auto file_result = co_await parse_file_part(headers);

                    if (!file_result)
                        co_return exceptions::make_unexpected(std::move(file_result.error()));

                    result.m_files.emplace(headers.m_name, std::move(file_result.value()));
                } else {
                    auto field_result = co_await parse_field_part();

                    if (!field_result)
                        co_return exceptions::make_unexpected(std::move(field_result.error()));

                    result.m_fields.emplace(headers.m_name, std::move(field_result.value()));
                }
            }

            co_return exceptions::make_unexpected(
                "Maximum iterations exceeded - possible infinite loop");
        }

       private:
        /**
         * @brief The read state enum.
         */
        enum struct e_read_state : std::uint8_t { e_success, e_error, e_eof };

        /**
         * @brief Reads more data from the file.
         *
         * @return An awaitable that resolves to an `e_read_state` enum.
         */
        CLUEAPI_NOINLINE shared::awaitable_t<e_read_state> read_more() {
            if (m_eof_reached)
                co_return e_read_state::e_eof;

            const auto processed_offset = m_buffer_view.data() - m_processing_buffer.data();

            const auto tail_size = m_buffer_view.size();

            if (processed_offset > 0) {
                std::memmove(m_processing_buffer.data(), m_buffer_view.data(), tail_size);

                m_processing_buffer.resize(tail_size);
            }

            const auto old_size = m_processing_buffer.size();

            m_processing_buffer.resize(old_size + m_cfg.m_chunk_size);

            boost::system::error_code ec{};

            auto bytes_read = co_await m_file.file_obj().async_read_some(
                boost::asio::buffer(m_processing_buffer.data() + old_size, m_cfg.m_chunk_size),
                boost::asio::redirect_error(boost::asio::use_awaitable, ec));

            m_processing_buffer.resize(old_size + bytes_read);

            if (ec == boost::asio::error::eof) {
                m_eof_reached = true;

                ec.clear();
            } else if (ec)
                co_return e_read_state::e_error;

            m_buffer_view = std::string_view(m_processing_buffer);

            co_return e_read_state::e_success;
        }

        /**
         * @brief Finds the first boundary in the buffer.
         *
         * @return An awaitable that resolves to a boolean indicating whether a boundary was found.
         *
         * @details This function searches for the first occurrence of the dash boundary in the
         * buffer and removes it from the buffer. It then returns a boolean indicating whether a
         * boundary was found.
         */
        CLUEAPI_NOINLINE shared::awaitable_t<bool> find_first_boundary() {
            while (true) {
                auto pos = m_buffer_view.find(m_dash_boundary);

                if (pos != std::string_view::npos) {
                    m_buffer_view.remove_prefix(pos + m_dash_boundary.length());

                    co_return true;
                }

                const auto keep_size =
                    std::max(m_dash_boundary.length(), m_crlf_dash_boundary.length()) - 1;

                if (m_buffer_view.length() > keep_size)
                    m_buffer_view.remove_prefix(m_buffer_view.length() - keep_size);

                if (m_eof_reached)
                    co_return false;

                auto read_result = co_await read_more();

                if (read_result == e_read_state::e_error || read_result == e_read_state::e_eof)
                    co_return false;
            }
        }

        /**
         * @brief Parses the headers of a part.
         *
         * @return An awaitable that resolves to a `headers_t` object containing the parsed headers.
         *
         * @details This function parses the headers of a part by searching for the first occurrence
         * of the CRLFCRLF sequence in the buffer. It then removes the headers from the buffer and
         * returns them as a `headers_t` object.
         */
        CLUEAPI_NOINLINE exceptions::expected_awaitable_t<headers_t> parse_part_headers() {
            while (!m_eof_reached) {
                auto pos = m_buffer_view.find("\r\n\r\n");

                if (pos != std::string_view::npos) {
                    auto headers_blob = m_buffer_view.substr(0, pos);

                    auto headers = parse_headers(headers_blob);

                    m_buffer_view.remove_prefix(pos + 4);

                    co_return exceptions::expected_t<headers_t>{std::move(headers)};
                }

                if (m_buffer_view.length() > 8192) {
                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Headers too large"));
                }

                auto read_result = co_await read_more();

                if (read_result == e_read_state::e_error) {
                    co_return exceptions::make_unexpected(
                        exceptions::io_error_t::make("Failed to read headers"));
                }
            }

            co_return exceptions::make_unexpected(
                exceptions::runtime_error_t::make("Headers section not found"));
        }

        /**
         * @brief Parses a file part.
         *
         * @param headers The headers of the part.
         *
         * @return An awaitable that resolves to a `types::file_t` object containing the parsed
         * file.
         *
         * @details This function parses a file part by creating a temporary file on disk or in
         * memory, depending on the configuration. It then reads the file data from the buffer and
         * writes it to the temporary file or vector. Finally, it returns a `types::file_t` object
         * containing the parsed file.
         */
        CLUEAPI_NOINLINE shared::awaitable_t<exceptions::expected_t<types::file_t>> parse_file_part(
            const headers_t& headers) {
            std::variant<std::vector<char>, tmp_file_t> storage = std::vector<char>();

            bool in_memory{};

            std::size_t part_size{};

            if (auto* vec = std::get_if<std::vector<char>>(&storage))
                vec->reserve(std::min(m_cfg.m_max_file_size_in_memory, m_cfg.m_chunk_size));

            while (true) {
                auto boundary_pos = find_boundary_in_buffer();

                if (boundary_pos != std::string_view::npos) {
                    auto final_chunk = m_buffer_view.substr(0, boundary_pos);

                    part_size += final_chunk.length();

                    auto write_result = co_await write_chunk_to_storage(
                        storage, in_memory, final_chunk, part_size, headers);

                    if (!write_result)
                        co_return exceptions::make_unexpected(std::move(write_result.error()));

                    storage = std::move(write_result.value());

                    in_memory = std::holds_alternative<std::vector<char>>(storage);

                    m_buffer_view.remove_prefix(boundary_pos + m_crlf_dash_boundary.length());

                    break;
                }

                const auto safe_chunk_size = calculate_safe_chunk_size();

                if (safe_chunk_size > 0) {
                    auto chunk = m_buffer_view.substr(0, safe_chunk_size);

                    part_size += chunk.length();

                    auto write_result = co_await write_chunk_to_storage(
                        storage, in_memory, chunk, part_size, headers);

                    if (!write_result)
                        co_return exceptions::make_unexpected(std::move(write_result.error()));

                    storage = std::move(write_result.value());

                    in_memory = std::holds_alternative<std::vector<char>>(storage);

                    m_buffer_view.remove_prefix(safe_chunk_size);
                }

                if (m_eof_reached) {
                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Part boundary not found"));
                }

                if (m_buffer_view.length() <= m_crlf_dash_boundary.length()) {
                    auto read_result = co_await read_more();

                    if (read_result == e_read_state::e_error) {
                        co_return exceptions::make_unexpected(
                            exceptions::io_error_t::make("Failed to read file part"));
                    }
                }
            }

            if (in_memory) {
                auto& vec = std::get<std::vector<char>>(storage);

                m_total_in_memory_size += vec.size();

                co_return types::file_t{
                    std::string{headers.m_file_name},
                    std::string{headers.m_content_type},
                    std::move(vec)};
            } else {
                auto& temp_storage = std::get<tmp_file_t>(storage);

                boost::system::error_code ec{};

                temp_storage.file_obj().close(ec);

                co_return types::file_t{
                    std::string{headers.m_file_name},
                    std::string{headers.m_content_type},
                    std::move(temp_storage.path())};
            }
        }

        /**
         * @brief Parses a field part.
         *
         * @return An awaitable that resolves to a `std::string` containing the parsed field value.
         *
         * @details This function parses a field part by searching for the first occurrence of the
         * boundary in the buffer and removing it from the buffer. It then returns the field value
         * as a `std::string`.
         */
        CLUEAPI_NOINLINE exceptions::expected_awaitable_t<std::string> parse_field_part() {
            std::string field_value{};

            field_value.reserve(1024);

            while (true) {
                auto boundary_pos = find_boundary_in_buffer();

                if (boundary_pos != std::string_view::npos) {
                    field_value.append(m_buffer_view.substr(0, boundary_pos));

                    m_buffer_view.remove_prefix(boundary_pos + m_crlf_dash_boundary.length());

                    break;
                }

                const auto safe_chunk_size = calculate_safe_chunk_size();

                if (safe_chunk_size > 0) {
                    field_value.append(m_buffer_view.substr(0, safe_chunk_size));

                    m_buffer_view.remove_prefix(safe_chunk_size);
                }

                if (m_eof_reached) {
                    co_return exceptions::make_unexpected(
                        exceptions::runtime_error_t::make("Field boundary not found. EOF reached"));
                }

                if (m_buffer_view.length() <= m_crlf_dash_boundary.length()) {
                    auto read_result = co_await read_more();

                    if (read_result == e_read_state::e_error) {
                        co_return exceptions::make_unexpected(
                            exceptions::io_error_t::make("Failed to read field"));
                    }
                }
            }

            co_return std::move(field_value);
        }

        /**
         * @brief Finds the boundary in the buffer.
         *
         * @return The index of the boundary in the buffer.
         */
        [[nodiscard]] CLUEAPI_INLINE std::size_t find_boundary_in_buffer() const {
            return m_buffer_view.find(m_crlf_dash_boundary);
        }

        /**
         * @brief Calculates the safe chunk size.
         *
         * @return The safe chunk size.
         *
         * @details This function calculates the safe chunk size by subtracting the length of the
         * CRLF dash boundary from the buffer length. If the result is less than or equal to zero,
         * it returns zero.
         */
        [[nodiscard]] CLUEAPI_INLINE std::size_t calculate_safe_chunk_size() const {
            if (m_buffer_view.length() <= m_crlf_dash_boundary.length())
                return 0;

            return m_buffer_view.length() - m_crlf_dash_boundary.length();
        }

        /**
         * @brief Checks if the part size should be stored in memory.
         *
         * @param part_size The size of the part.
         *
         * @return `true` if the part size exceeds the maximum allowed size for in-memory storage,
         * `false` otherwise.
         */
        [[nodiscard]] CLUEAPI_INLINE bool should_use_file_storage(std::size_t part_size) const {
            return part_size > m_cfg.m_max_file_size_in_memory ||
                   m_total_in_memory_size + part_size > m_cfg.m_max_files_size_in_memory;
        }

        /**
         * @brief Writes a chunk to storage.
         *
         * @param storage The storage object to write to.
         * @param in_memory A reference to a boolean indicating whether the storage is in memory.
         * @param chunk The chunk to write.
         * @param total_part_size The total size of the part.
         * @param headers The headers of the part.
         *
         * @return An awaitable that resolves to a `types::file_t` object containing the parsed
         * file.
         *
         * @details This function writes a chunk to storage, either in memory or on disk. It also
         * updates the total size of the part and the in-memory storage size. It returns a
         * `types::file_t` object containing the parsed file.
         */
        CLUEAPI_NOINLINE
        exceptions::expected_awaitable_t<std::variant<std::vector<char>, tmp_file_t>>
        write_chunk_to_storage(
            std::variant<std::vector<char>, tmp_file_t>& storage,
            bool& in_memory,
            std::string_view chunk,
            std::size_t total_part_size,
            const headers_t& headers) {
            if (chunk.empty()) {
                co_return exceptions::expected_t<std::variant<std::vector<char>, tmp_file_t>>{
                    std::move(storage)};
            }

            if (in_memory && should_use_file_storage(total_part_size)) {
                auto temp_storage_result = co_await create_temp_file_storage(storage, chunk);

                if (!temp_storage_result)
                    co_return exceptions::make_unexpected(std::move(temp_storage_result.error()));

                co_return exceptions::expected_t<std::variant<std::vector<char>, tmp_file_t>>{
                    std::move(temp_storage_result.value())};
            }

            if (in_memory) {
                auto& vec = std::get<std::vector<char>>(storage);

                vec.insert(vec.end(), chunk.begin(), chunk.end());
            } else {
                auto& temp_storage = std::get<tmp_file_t>(storage);

                co_await boost::asio::async_write(
                    temp_storage.file_obj(),
                    boost::asio::buffer(chunk),
                    boost::asio::use_awaitable);
            }

            co_return exceptions::expected_t<std::variant<std::vector<char>, tmp_file_t>>{
                std::move(storage)};
        }

        /**
         * @brief Creates a temporary file storage.
         *
         * @param current_storage The current storage object.
         * @param additional_chunk The additional chunk to write.
         *
         * @return An awaitable that resolves to a `std::variant<std::vector<char>, tmp_file_t>`
         * object containing the parsed file.
         *
         * @details This function creates a temporary file storage by appending the additional chunk
         * to the current storage and returning a `std::variant<std::vector<char>, tmp_file_t>`
         * object containing the parsed file.
         */
        CLUEAPI_NOINLINE
        exceptions::expected_awaitable_t<std::variant<std::vector<char>, tmp_file_t>>
        create_temp_file_storage(
            const std::variant<std::vector<char>, tmp_file_t>& current_storage,
            std::string_view additional_chunk) {
            auto executor = co_await boost::asio::this_coro::executor;

            auto temp_path = boost::filesystem::temp_directory_path() /
                             boost::filesystem::unique_path("clueapi-%%%%-%%%%.tmp");

            tmp_file_t new_storage{executor, temp_path};

            boost::system::error_code ec{};

            new_storage.file_obj().open(
                temp_path.string(),
                boost::asio::stream_file::write_only | boost::asio::stream_file::create,
                ec);

            if (ec) {
                co_return exceptions::make_unexpected(
                    exceptions::io_error_t::make("Failed to create temp file: {}", ec.message()));
            }

            if (const auto* vec = std::get_if<std::vector<char>>(&current_storage)) {
                if (!vec->empty()) {
                    co_await boost::asio::async_write(
                        new_storage.file_obj(),
                        boost::asio::buffer(*vec),
                        boost::asio::use_awaitable);
                }
            }

            if (!additional_chunk.empty()) {
                co_await boost::asio::async_write(
                    new_storage.file_obj(),
                    boost::asio::buffer(additional_chunk),
                    boost::asio::use_awaitable);
            }

            co_return exceptions::expected_t<std::variant<std::vector<char>, tmp_file_t>>{
                std::move(new_storage)};
        }

       private:
        /**
         * @brief The temporary file object.
         */
        tmp_file_t m_file;

        /**
         * @brief The path to the file.
         */
        boost::filesystem::path m_file_path;

        /**
         * @brief The configuration for the multipart parser.
         */
        cfg_t m_cfg;

        /**
         * @brief Whether the end of the file has been reached.
         */
        bool m_eof_reached{false};

        /**
         * @brief The dash boundary string.
         */
        std::string_view m_dash_boundary;

        /**
         * @brief The CRLF dash boundary string.
         */
        std::string_view m_crlf_dash_boundary;

        /**
         * @brief The total size of the part in memory.
         */
        std::size_t m_total_in_memory_size{};

        /**
         * @brief The processing buffer.
         */
        std::string m_processing_buffer;

        /**
         * @brief The buffer view.
         */
        std::string_view m_buffer_view;
    };
} // namespace clueapi::http::multipart::detail

#endif // CLUEAPI_HTTP_MULTIPART_DETAIL_FILE_PARSER_HXX