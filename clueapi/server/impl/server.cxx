/**
 * @file server.cxx
 *
 * @brief Implements the main server class.
 */

#include <clueapi.hxx>

namespace clueapi::server {
    class c_server::c_impl {
       public:
        /**
         * @brief The state of the server instance.
         *
         * @details This struct represents the state of the server instance.
         *
         * @internal
         */
        struct state_t {
            enum e_state : std::uint8_t { stopped, starting, running, stopping };

            state_t() : m_state{e_state::stopped} {
            }

            state_t(e_state state) : m_state{state} {
            }

           public:
            [[nodiscard]] e_state current(bool need_log = false) const {
                auto ret = m_state.load(std::memory_order_acquire);

                if (need_log)
                    state_t::log(ret);

                return ret;
            }

            [[nodiscard]] e_state current(std::memory_order m) const {
                auto ret = m_state.load(m);

                return ret;
            }

            void update(e_state state) {
                m_state.store(state, std::memory_order_release);

                CLUEAPI_LOG_TRACE(
                    "Updated SERVER state to '{}'",

                    state_t::to_str(state));
            }

            bool compare_exchange_strong(e_state expected, e_state desired) {
                auto success =
                    m_state.compare_exchange_strong(expected, desired, std::memory_order_acq_rel);

                if (success) {
                    CLUEAPI_LOG_TRACE(
                        "Updated and compared SERVER state from '{}' to '{}'",

                        state_t::to_str(expected),
                        state_t::to_str(desired));
                }

                return success;
            }

            void log() const {
                CLUEAPI_LOG_TRACE("Current SERVER state: '{}'", state_t::to_str(current()));
            }

            static void log(e_state state) {
                CLUEAPI_LOG_TRACE("Current SERVER state: '{}'", state_t::to_str(state));
            }

            [[nodiscard]] std::string_view to_str() const {
                switch (current()) {
                    case e_state::stopped:
                        return "stopped";
                    case e_state::starting:
                        return "starting";
                    case e_state::running:
                        return "running";
                    case e_state::stopping:
                        return "stopping";
                    default:
                        return "unknown";
                }
            }

            static std::string_view to_str(e_state state) {
                switch (state) {
                    case e_state::stopped:
                        return "stopped";
                    case e_state::starting:
                        return "starting";
                    case e_state::running:
                        return "running";
                    case e_state::stopping:
                        return "stopping";
                    default:
                        return "unknown";
                }
            }

           private:
            std::atomic<e_state> m_state;
        };

        c_impl(
            c_server* self,
            c_clueapi& clueapi,
            shared::io_ctx_pool_t& io_ctx_pool,
            cfg::cfg_t cfg) noexcept
            : m_self{self},
              m_clueapi{clueapi},
              m_io_ctx_pool{io_ctx_pool},
              m_cfg{std::move(cfg)},
              m_state{state_t::stopped},
              m_clients{cfg.m_server.m_acceptor.m_max_connections},
              m_clients_storage{cfg.m_server.m_acceptor.m_max_connections},
              m_active_connections{0},
              m_total_connections{0} {
        }

        ~c_impl() noexcept {
            try {
                stop();
            } catch (...) {
                // ...
            }
        }

       public:
        void start() {
            CLUEAPI_LOG_TRACE("Trying to start the server");

            auto expected = state_t::stopped;

            if (!m_state.compare_exchange_strong(expected, state_t::starting)) {
                CLUEAPI_LOG_WARNING("Server start called but not in stopped state");

                return;
            }

            {
                m_self->set_keep_alive_timeout(fmt::format(
                    "timeout={}",

                    std::chrono::duration_cast<std::chrono::seconds>(
                        m_cfg.m_http.m_keep_alive_timeout)
                        .count()));
            }

            try {
                init_clients();

                setup_acceptor();

                start_accept_loops();

                CLUEAPI_LOG_INFO(
                    "The server has successfully started running on {}:{}",

                    m_cfg.m_host,
                    m_cfg.m_port);
            } catch (const std::exception& e) {
                CLUEAPI_LOG_ERROR("Failed to start server: {}", e.what());

                destroy_acceptor();

                m_state.update(state_t::stopped);

                throw;
            }
        }

        [[nodiscard]] bool is_running(
            std::memory_order m = std::memory_order_acquire) const noexcept {
            return m_state.current(m) == state_t::running;
        }

        void stop() {
            auto curr_state = m_state.current();

            if (curr_state == state_t::stopped || curr_state == state_t::stopping) {
                CLUEAPI_LOG_TRACE("Server already stopped/stopping — skipping");

                return;
            }

            auto expected = state_t::running;

            if (!m_state.compare_exchange_strong(expected, state_t::stopping))
                return;

            {
                destroy_acceptor();

                destroy_clients();
            }

            m_state.update(state_t::stopped);

            CLUEAPI_LOG_INFO("Server successfully stopped execution.");

            CLUEAPI_LOG_INFO("Total connections handled: {}", m_total_connections.load());
        }

        [[nodiscard]] std::size_t get_total_connections() const noexcept {
            return m_total_connections.load(std::memory_order_relaxed);
        }

        [[nodiscard]] std::size_t get_active_connections() const noexcept {
            return m_active_connections.load(std::memory_order_relaxed);
        }

       private:
        void start_accept_loops() {
            if (!m_acceptor)
                throw exceptions::exception_t(
                    "Cannot start accept loops: acceptor not initialized");

            auto* io_ctx = m_io_ctx_pool.io_ctx();

            if (!io_ctx)
                throw exceptions::exception_t("No I/O context available for acceptor");

            // Set the state to running before starting the accept loops
            m_state.update(state_t::running);

            if (m_cfg.m_server.m_acceptor.m_reuse_port) {
#ifdef SO_REUSEPORT
                const auto acceptor_count =
                    std::clamp((m_cfg.m_workers + 3u) / 4u, 1u, std::max(1u, m_cfg.m_workers / 2u));

                CLUEAPI_LOG_DEBUG(
                    "Starting {} accept loops for {} worker threads",

                    acceptor_count,
                    m_cfg.m_workers);

                for (std::size_t i{}; i < acceptor_count; i++) {
                    boost::asio::co_spawn(*io_ctx, accept_loop(i), boost::asio::detached);

                    io_ctx = m_io_ctx_pool.io_ctx();

                    if (!io_ctx)
                        throw exceptions::exception_t("No I/O context available for acceptor");
                }
#else
                boost::asio::co_spawn(*io_ctx, accept_loop(0), boost::asio::detached);
#endif
            } else
                boost::asio::co_spawn(*io_ctx, accept_loop(0), boost::asio::detached);
        }

        shared::awaitable_t<void> accept_loop(std::size_t loop_id) {
            while (is_running(std::memory_order_relaxed)) {
                try {
                    boost::system::error_code ec;

                    auto* io_ctx = m_io_ctx_pool.io_ctx();

                    if (!io_ctx) {
                        io_ctx = m_io_ctx_pool.io_ctx();

                        if (!io_ctx)
                            throw exceptions::exception_t("No I/O context available for acceptor");
                    }

                    boost::asio::ip::tcp::socket socket(*io_ctx);

                    co_await m_acceptor->async_accept(
                        socket,

                        boost::asio::redirect_error(boost::asio::use_awaitable, ec));

                    if (handle_accept_error(ec, loop_id)) {
                        close_socket_gracefully(socket);

                        break;
                    }

                    if (!is_running(std::memory_order_relaxed)) {
                        CLUEAPI_LOG_TRACE("Accept loop {} shutting down, closing socket", loop_id);

                        close_socket_gracefully(socket);

                        break;
                    }

                    log_new_connection(socket, loop_id);

                    boost::asio::co_spawn(
                        *io_ctx,

                        handle_client_connection(std::move(socket)),

                        boost::asio::detached);

                    m_total_connections.fetch_add(1, std::memory_order_relaxed);
                } catch (const std::exception& e) {
                    if (!is_running(std::memory_order_relaxed)) {
                        CLUEAPI_LOG_TRACE(
                            "Accept loop {} exception during shutdown: {}", loop_id, e.what());

                        co_return;
                    }

                    CLUEAPI_LOG_ERROR("Exception in accept loop {}: {}", loop_id, e.what());
                }
            }

            CLUEAPI_LOG_TRACE("Accept loop {} completed", loop_id);

            co_return;
        }

        shared::awaitable_t<void> handle_client_connection(boost::asio::ip::tcp::socket socket) {
            std::int32_t socket_handle{};

            client::client_t* client{};

            try {
                if (socket.is_open())
                    socket_handle = socket.native_handle();

                client = acquire_client();

                if (!client) {
                    CLUEAPI_LOG_WARNING(
                        "No available clients in pool - rejecting connection (id: {})",

                        socket_handle);

                    close_socket_gracefully(socket);

                    co_return;
                }

                update_socket_settings(socket);

                if (!client->prepare_for_connection(std::move(socket))) {
                    CLUEAPI_LOG_ERROR(
                        "Failed to prepare client for connection (id: {})", socket_handle);

                    release_client(client);

                    co_return;
                }

                CLUEAPI_LOG_TRACE("Client prepared for connection (id: {})", socket_handle);

                m_active_connections.fetch_add(1, std::memory_order_relaxed);

                co_await client->start();
            } catch (const std::exception& e) {
                CLUEAPI_LOG_ERROR(
                    "Exception in client connection handler (id: {}): {}", socket_handle, e.what());
            } catch (...) {
                CLUEAPI_LOG_ERROR(
                    "Unknown exception in client connection handler (id: {})", socket_handle);
            }

            if (client)
                release_client(client);

            m_active_connections.fetch_sub(1, std::memory_order_relaxed);

            CLUEAPI_LOG_TRACE("Client connection handler completed (id: {})", socket_handle);

            co_return;
        }

        void update_socket_settings(boost::asio::ip::tcp::socket& socket) {
            boost::system::error_code ec;

            if (m_cfg.m_socket.m_tcp_no_delay) {
                boost::asio::ip::tcp::no_delay no_delay_option(true);

                socket.set_option(no_delay_option, ec);

#if defined(TCP_QUICKACK) && defined(__linux__)
                constexpr std::int32_t quickack = 1;

                auto res = setsockopt(
                    socket.native_handle(), IPPROTO_TCP, TCP_QUICKACK, &quickack, sizeof(quickack));

                if (res != 0)
                    CLUEAPI_LOG_WARNING("Failed to set TCP_QUICKACK option");
#endif
            }

            if (m_cfg.m_socket.m_rcv_buf_size) {
                boost::asio::socket_base::receive_buffer_size rcv_buf_size(
                    m_cfg.m_socket.m_rcv_buf_size);

                socket.set_option(rcv_buf_size, ec);

                if (ec) {
                    CLUEAPI_LOG_WARNING(
                        "Failed to set RCV_BUF_SIZE option: {}",

                        ec.message());
                }
            }

            if (m_cfg.m_socket.m_snd_buf_size) {
                boost::asio::socket_base::send_buffer_size snd_buf_size(
                    m_cfg.m_socket.m_snd_buf_size);

                socket.set_option(snd_buf_size, ec);

                if (ec) {
                    CLUEAPI_LOG_WARNING(
                        "Failed to set SND_BUF_SIZE option: {}",

                        ec.message());
                }
            }

            if (m_cfg.m_socket.m_tcp_keep_alive) {
                socket.set_option(boost::asio::socket_base::keep_alive(true), ec);

                if (ec) {
                    CLUEAPI_LOG_WARNING(
                        "Failed to enable TCP_KEEPALIVE: {}",

                        ec.message());
                }
            }
        }

        bool handle_accept_error(const boost::system::error_code& ec, std::size_t loop_id) {
            if (!ec)
                return false;

            if (ec == boost::asio::error::operation_aborted ||
                ec == boost::asio::error::operation_not_supported) {
                CLUEAPI_LOG_TRACE("Accept operation aborted in loop {}", loop_id);

                return true;
            }

            if (ec == boost::asio::error::no_descriptors || ec == boost::asio::error::no_memory) {
                CLUEAPI_LOG_ERROR(
                    "Resource exhaustion in accept loop {}: {}", loop_id, ec.message());

                return true;
            }

            CLUEAPI_LOG_ERROR("Accept error in loop {}: {}", loop_id, ec.message());

            return false;
        }

        void log_new_connection(const boost::asio::ip::tcp::socket& socket, std::size_t loop_id) {
            try {
                boost::system::error_code ec{};

                auto remote = socket.remote_endpoint(ec);

                if (!ec) {
                    CLUEAPI_LOG_TRACE(
                        "Accept loop {} accepted connection from {}:{}",

                        loop_id,
                        remote.address().to_string(),
                        remote.port());
                } else {
                    CLUEAPI_LOG_TRACE(
                        "Accept loop {} accepted connection (failed to get remote endpoint: {})",

                        loop_id,
                        ec.message());
                }
            } catch (...) {
                CLUEAPI_LOG_TRACE(
                    "Accept loop {} accepted connection (failed to log details)", loop_id);
            }
        }

        void close_socket_gracefully(boost::asio::ip::tcp::socket& socket) {
            if (!socket.is_open())
                return;

            try {
                boost::system::error_code ec;

                socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

                if (ec && ec != boost::asio::error::not_connected)
                    CLUEAPI_LOG_TRACE("Failed to shutdown socket: {}", ec.message());

                socket.close(ec);

                if (ec)
                    CLUEAPI_LOG_TRACE("Failed to close socket: {}", ec.message());
            } catch (...) {
                // ...
            }
        }

        void setup_acceptor() {
            CLUEAPI_LOG_TRACE("Setting up acceptor...");

            if (m_acceptor) {
                CLUEAPI_LOG_WARNING("Server already has an acceptor");

                return;
            }

            if (!is_port_available(m_cfg.m_host, m_cfg.m_port)) {
                throw exceptions::exception_t(
                    "Cannot start server: port {} on host {} is already in use",

                    m_cfg.m_port,
                    m_cfg.m_host);
            }

            boost::system::error_code ec;

            auto addr = boost::asio::ip::make_address(m_cfg.m_host, ec);

            if (ec)
                throw exceptions::exception_t(
                    "Invalid host address '{}': {}", m_cfg.m_host, ec.message());

            auto port = static_cast<std::uint16_t>(std::stoul(m_cfg.m_port));

            boost::asio::ip::tcp::endpoint endpoint(addr, port);

            CLUEAPI_LOG_TRACE("Creating TCP acceptor on {}:{}", addr.to_string(), port);

            auto* io_ctx = m_io_ctx_pool.io_ctx();

            if (!io_ctx)
                throw exceptions::exception_t("No I/O context available for acceptor");

            m_acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(*io_ctx);

            configure_acceptor(endpoint);
        }

        void configure_acceptor(boost::asio::ip::tcp::endpoint& endpoint) {
            if (!m_acceptor) {
                CLUEAPI_LOG_WARNING("Server does not have an acceptor");

                return;
            }

            boost::system::error_code ec;

            m_acceptor->open(endpoint.protocol(), ec);

            if (ec)
                throw exceptions::exception_t("Failed to open acceptor: {}", ec.message());

            const auto& acceptor_cfg = m_cfg.m_server.m_acceptor;

            if (acceptor_cfg.m_reuse_address) {
                m_acceptor->set_option(boost::asio::socket_base::reuse_address(true), ec);

                if (ec)
                    CLUEAPI_LOG_WARNING("Failed to set SO_REUSEADDR: {}", ec.message());
            }

            if (acceptor_cfg.m_reuse_port) {
#ifdef SO_REUSEPORT
                auto socket_opt =
                    boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT>(true);

                m_acceptor->set_option(socket_opt, ec);

                if (ec)
                    CLUEAPI_LOG_WARNING("Failed to set SO_REUSEPORT: {}", ec.message());
#endif
            }

            if (acceptor_cfg.m_tcp_fast_open) {
#ifdef TCP_FASTOPEN
                auto socket_opt =
                    boost::asio::detail::socket_option::boolean<SOL_TCP, TCP_FASTOPEN>(true);

                m_acceptor->set_option(socket_opt, ec);

                if (ec)
                    CLUEAPI_LOG_WARNING("Failed to set TCP_FASTOPEN: {}", ec.message());
#endif
            }

            if (acceptor_cfg.m_nonblocking) {
                m_acceptor->non_blocking(true, ec);

                if (ec)
                    CLUEAPI_LOG_WARNING("Failed to set non-blocking mode: {}", ec.message());
            } else {
                m_acceptor->non_blocking(false, ec);

                if (ec)
                    CLUEAPI_LOG_WARNING("Failed to set blocking mode: {}", ec.message());
            }

            m_acceptor->bind(endpoint, ec);

            if (ec)
                throw exceptions::exception_t(
                    "Failed to bind to {}: {}", endpoint.address().to_string(), ec.message());

            m_acceptor->listen(
                static_cast<std::int32_t>(m_cfg.m_server.m_acceptor.m_max_connections), ec);

            if (ec)
                throw exceptions::exception_t("Failed to listen: {}", ec.message());

            CLUEAPI_LOG_DEBUG(
                "Acceptor successfully configured on {}:{}",

                endpoint.address().to_string(),
                endpoint.port());
        }

        void init_clients() {
            const auto num_clients = m_cfg.m_server.m_acceptor.m_max_connections;

            CLUEAPI_LOG_TRACE("Initializing client pool with {} clients", num_clients);

            if (num_clients == 0)
                throw exceptions::exception_t(
                    "Invalid max_connections configuration: cannot be zero");

            m_clients_storage.clear();

            m_clients_storage.reserve(num_clients);

            std::size_t created_clients{};

            for (std::size_t i{}; i < num_clients; i++) {
                try {
                    auto client = std::make_unique<client::client_t>(*m_self, m_cfg);

                    if (!client->is_ready_for_reuse()) {
                        CLUEAPI_LOG_ERROR("Created client {} is not ready for reuse", i);

                        continue;
                    }

                    m_clients_storage.emplace_back(std::move(client));

                    if (!m_clients.push(m_clients_storage.back().get())) {
                        CLUEAPI_LOG_ERROR("Failed to add client {} to pool", i);

                        m_clients_storage.pop_back();

                        break;
                    }

                    ++created_clients;
                } catch (const std::exception& e) {
                    CLUEAPI_LOG_ERROR("Failed to create client {}: {}", i, e.what());
                } catch (...) {
                    CLUEAPI_LOG_ERROR("Unknown error creating client {}", i);
                }
            }

            if (created_clients == 0)
                throw exceptions::exception_t("Failed to create any clients for the pool");

            CLUEAPI_LOG_DEBUG(
                "Client pool initialized with {}/{} clients", created_clients, num_clients);
        }

        client::client_t* acquire_client() {
            client::client_t* client{};

            std::size_t attempts{};

            constexpr std::size_t k_max_attempts = 3;

            while (attempts < k_max_attempts && m_clients.pop(client)) {
                ++attempts;

                if (!client) {
                    CLUEAPI_LOG_WARNING("Retrieved null client from pool (attempt {})", attempts);

                    continue;
                }

                if (!client->is_ready_for_reuse()) {
                    CLUEAPI_LOG_WARNING(
                        "Retrieved non-idle client from pool, attempting cleanup (attempt {})",

                        attempts);

                    client->return_to_pool();

                    if (client->is_ready_for_reuse()) {
                        if (!m_clients.push(client))
                            CLUEAPI_LOG_WARNING("Failed to return cleaned client to pool");
                    }

                    continue;
                }

                CLUEAPI_LOG_TRACE("Successfully acquired client from pool");

                return client;
            }

            CLUEAPI_LOG_TRACE("Failed to acquire client from pool after {} attempts", attempts);

            return nullptr;
        }

        void release_client(client::client_t* client) {
            if (!client) {
                CLUEAPI_LOG_WARNING("Attempting to release null client");

                return;
            }

            try {
                client->return_to_pool();

                if (!client->is_ready_for_reuse()) {
                    CLUEAPI_LOG_ERROR(
                        "Client failed to return to idle state - not returning to pool");

                    return;
                }

                if (!m_clients.push(client)) {
                    CLUEAPI_LOG_WARNING("Failed to return client to pool - pool may be full");

                    return;
                }

                CLUEAPI_LOG_TRACE("Successfully returned client to pool");
            } catch (const std::exception& e) {
                CLUEAPI_LOG_ERROR("Exception while releasing client: {}", e.what());
            } catch (...) {
                CLUEAPI_LOG_ERROR("Unknown exception while releasing client");
            }
        }

        void destroy_clients() {
            CLUEAPI_LOG_TRACE("Destroying client pool");

            for (const auto& client : m_clients_storage) {
                if (!client || client->is_ready_for_reuse())
                    continue;

                client->return_to_pool();
            }

            if (m_active_connections.load(std::memory_order_acquire) > 0) {
                CLUEAPI_LOG_TRACE(
                    "Waiting for {} active client(s) to finish", m_active_connections.load());

                auto deadline = std::chrono::steady_clock::now() +
                                m_cfg.m_server.m_deadline_for_destroying_clients;

                while (m_active_connections.load(std::memory_order_acquire) > 0) {
                    if (std::chrono::steady_clock::now() > deadline) {
                        CLUEAPI_LOG_WARNING(
                            "Shutdown timeout exceeded. {} client(s) did not stop.",

                            m_active_connections.load());

                        break;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds{25});
                }
            }

            std::size_t clients_in_pool{};

            {
                client::client_t* client{};

                while (m_clients.pop(client))
                    clients_in_pool++;
            }

            if (clients_in_pool == m_clients_storage.size()) {
                CLUEAPI_LOG_TRACE("All clients successfully destroyed");
            } else
                CLUEAPI_LOG_WARNING("Client pool is not empty after destroying clients");

            m_clients_storage.clear();
        }

        void destroy_acceptor() {
            if (!m_acceptor) {
                CLUEAPI_LOG_TRACE("Acceptor is already destroyed or not initialized");

                return;
            }

            if (!m_acceptor->is_open()) {
                CLUEAPI_LOG_TRACE("Acceptor is already closed");

                m_acceptor.reset();

                return;
            }

            boost::system::error_code ec;

            m_acceptor->cancel(ec);

            if (ec)
                CLUEAPI_LOG_WARNING("Failed to cancel acceptor: {}", ec.message());

            m_acceptor->close(ec);

            if (ec)
                CLUEAPI_LOG_WARNING("Failed to close acceptor: {}", ec.message());

            m_acceptor.reset();

            CLUEAPI_LOG_DEBUG("Acceptor successfully destroyed");
        }

        [[nodiscard]] bool is_port_available(const std::string& host, const std::string& port) {
            boost::asio::io_context io{};

            boost::asio::ip::tcp::endpoint endpoint{};

            boost::system::error_code ec{};

            auto addr = boost::asio::ip::make_address(host, ec);

            if (ec)
                return false;

            endpoint =
                boost::asio::ip::tcp::endpoint(addr, static_cast<std::uint16_t>(std::stoul(port)));

            boost::asio::ip::tcp::acceptor test_acceptor(io);

            test_acceptor.open(endpoint.protocol(), ec);

            if (ec)
                return false;

            test_acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);

            if (ec)
                return false;

            test_acceptor.bind(endpoint, ec);

            if (ec)
                return false;

            test_acceptor.close(ec);

            return true;
        }

       private:
        c_server* m_self;

        c_clueapi& m_clueapi;

        shared::io_ctx_pool_t& m_io_ctx_pool;

        boost::lockfree::queue<client::client_t*> m_clients;

        std::vector<std::unique_ptr<client::client_t>> m_clients_storage;

        std::unique_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;

        std::atomic<std::size_t> m_active_connections;

        std::atomic<std::size_t> m_total_connections;

        state_t m_state;

        cfg::cfg_t m_cfg;
    };

    c_server::c_server(
        c_clueapi& clueapi,
        shared::io_ctx_pool_t& io_ctx_pool,
        middleware::next_t& middleware_chain,
        cfg::cfg_t cfg)
        : m_impl{std::make_unique<c_impl>(this, clueapi, io_ctx_pool, std::move(cfg))},
          m_clueapi(clueapi),
          m_middleware_chain{middleware_chain} {
    }

    c_server::~c_server() = default;

    void c_server::start() {
        m_impl->start();
    }

    void c_server::stop() {
        m_impl->stop();
    }

    bool c_server::is_running(std::memory_order m) const noexcept {
        return m_impl->is_running(m);
    }
} // namespace clueapi::server