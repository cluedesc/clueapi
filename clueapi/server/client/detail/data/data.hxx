/**
 * @file data.hxx
 *
 * @brief Contains the implementation of the client-related functionality.
 */

#ifndef CLUEAPI_SERVER_CLIENT_DETAIL_DATA_HXX
#define CLUEAPI_SERVER_CLIENT_DETAIL_DATA_HXX

namespace clueapi::server::client::detail {
    /**
     * @struct data_t
     *
     * @brief Contains the state of a client connection.
     */
    struct data_t {
        /**
         * @enum e_state
         *
         * @brief The state of a client connection.
         */
        enum struct e_state : std::uint8_t { idle, active, cleanup };

        /**
         * @brief Constructs a new data object.
         *
         * @param buffer_size The size of the buffer to use.
         *
         * @note The buffer size is used to reserve memory for the buffer.
         */
        CLUEAPI_INLINE data_t(std::size_t buffer_size) : m_buffer{buffer_size} {
        }

        CLUEAPI_INLINE ~data_t() {
            force_cleanup();
        }

        // Copy constructor
        CLUEAPI_INLINE data_t(const data_t&) = delete;

        // Copy assignment operator
        CLUEAPI_INLINE data_t& operator=(const data_t&) = delete;

        // Move constructor
        CLUEAPI_INLINE data_t(data_t&&) = delete;

        // Move assignment operator
        CLUEAPI_INLINE data_t& operator=(data_t&&) = delete;

       public:
        /**
         * @brief Initializes the client with a new socket.
         *
         * @param socket The socket to use.
         *
         * @return True if the initialization was successful, false otherwise.
         *
         * @note This method should only be called once per client.
         */
        CLUEAPI_INLINE bool init(boost::asio::ip::tcp::socket&& socket) {
            if (m_state != e_state::idle) {
                CLUEAPI_LOG_WARNING("Attempting to initialize client in non-idle state");

                return false;
            }

            try {
                m_state = e_state::active;

                m_socket.emplace(std::move(socket));

                if (m_socket && m_socket->is_open()) {
                    const auto& executor = m_socket->get_executor();

                    if (!m_timeout_timer)
                        m_timeout_timer.emplace(executor);

                    return true;
                }

                reset_to_idle();

                return false;
            } catch (const std::exception& e) {
                CLUEAPI_LOG_ERROR("Failed to initialize client: {}", e.what());

                reset_to_idle();

                return false;
            }
        }

        /**
         * @brief Resets the client to the idle state.
         */
        CLUEAPI_INLINE void reset_to_idle() {
            if (m_state == e_state::idle)
                return;

            m_state = e_state::cleanup;

            if (m_socket && m_socket->is_open()) {
                boost::system::error_code ec{};

                m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

                if (ec && ec != boost::asio::error::not_connected)
                    CLUEAPI_LOG_TRACE("Failed to shutdown socket: {}", ec.message());

                m_socket->cancel(ec);

                if (ec)
                    CLUEAPI_LOG_TRACE("Failed to cancel socket operations: {}", ec.message());

                m_socket->close(ec);

                if (ec)
                    CLUEAPI_LOG_TRACE("Failed to close socket: {}", ec.message());
            }

            m_socket.reset();

            if (m_timeout_timer) {
                m_timeout_timer->cancel();

                m_timeout_timer.reset();
            }

            m_buffer.consume(m_buffer.size());

            m_request.reset();

            m_response_data.reset();

            m_beast_request.clear();

            m_should_close = false;

            m_state = e_state::idle;
        }

        /**
         * @brief Forces the client to the cleanup state.
         */
        CLUEAPI_INLINE void force_cleanup() {
            if (m_state == e_state::idle)
                return;

            try {
                m_state = e_state::cleanup;

                if (m_socket) {
                    boost::system::error_code ec{};

                    m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

                    if (ec && ec != boost::asio::error::not_connected)
                        CLUEAPI_LOG_TRACE("Failed to shutdown socket: {}", ec.message());

                    m_socket->cancel(ec);

                    if (ec)
                        CLUEAPI_LOG_TRACE("Failed to cancel socket operations: {}", ec.message());

                    m_socket->close(ec);

                    if (ec)
                        CLUEAPI_LOG_TRACE("Failed to close socket: {}", ec.message());

                    m_socket.reset();
                }

                if (m_timeout_timer) {
                    m_timeout_timer->cancel();

                    m_timeout_timer.reset();
                }

                m_buffer.clear();

                m_request.reset();

                m_response_data.reset();

                m_beast_request.clear();

                m_should_close = false;

                m_state = e_state::idle;
            } catch (...) {
                // ...
            }
        }

        /**
         * @brief Cuts the buffer to the specified size.
         *
         * @param size The new size of the buffer.
         *
         * @note This method is used to reduce the memory footprint of the buffer.
         */
        CLUEAPI_INLINE void cut_buffer(std::size_t size) {
            if (m_buffer.capacity() < size * 2)
                return;

            boost::beast::flat_buffer tmp{};

            std::swap(m_buffer, tmp);

            m_buffer.reserve(size);
        }

       public:
        /**
         * @brief Gets the current state of the client.
         *
         * @return True if the client is idle, false otherwise.
         */
        CLUEAPI_INLINE bool is_idle() const {
            return m_state == e_state::idle;
        }

        /**
         * @brief Gets the current state of the client.
         *
         * @return True if the client is active, false otherwise.
         */
        CLUEAPI_INLINE bool is_active() const {
            return m_state == e_state::active;
        }

        /**
         * @brief Gets the current state of the client.
         *
         * @return True if the client is connected, false otherwise.
         */
        CLUEAPI_INLINE bool is_connected() const {
            return m_socket && m_socket->is_open() && m_state == e_state::active;
        }

       public:
        /**
         * @brief The current state of the client.
         */
        e_state m_state{e_state::idle};

        /**
         * @brief If true, the client should be closed.
         */
        bool m_should_close{false};

        /**
         * @brief The socket of the client.
         */
        std::optional<boost::asio::ip::tcp::socket> m_socket;

        /**
         * @brief The timeout timer of the client.
         */
        std::optional<boost::asio::steady_timer> m_timeout_timer;

        /**
         * @brief The buffer of the client.
         */
        boost::beast::flat_buffer m_buffer;

        /**
         * @brief The request of the client.
         */
        http::types::request_t m_request;

        /**
         * @brief The response data of the client.
         */
        http::types::response_t m_response_data;

        /**
         * @brief The BEAST request of the client.
         */
        boost::beast::http::message<true, boost::beast::http::string_body> m_beast_request;
    };
} // namespace clueapi::server::client::detail

#endif // CLUEAPI_SERVER_CLIENT_DETAIL_DATA_HXX