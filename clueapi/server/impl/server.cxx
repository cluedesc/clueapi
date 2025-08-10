/**
 * @file server.cxx
 *
 * @brief Implements the main server class.
 */

#include <clueapi.hxx>

namespace clueapi::server {
    class c_server::c_impl {
      public:
        c_impl(
            c_clueapi& clueapi, c_server* self,

            std::string host, std::int32_t port
        )
            : m_clueapi(clueapi),
              m_acceptor(m_io_ctx),
              m_port(port),
              m_self(self),
              m_client_pool(clueapi.cfg().m_server.m_max_connections) {
            boost::system::error_code ec{};

            const auto& cfg = m_clueapi.cfg();

            auto addr = boost::asio::ip::make_address(host, ec);

            if (ec) {
                throw exceptions::exception_t(
                    "Failed to parse address '{}': {}",

                    host, ec.message()
                );
            }

            boost::asio::ip::tcp::endpoint endpoint(addr, m_port);

            m_acceptor.open(endpoint.protocol(), ec);

            if (ec) {
                throw exceptions::exception_t(
                    "Failed to open acceptor: {}",

                    ec.message()
                );
            }

            const auto& acceptor_cfg = cfg.m_server.m_acceptor;

            m_acceptor.non_blocking(acceptor_cfg.m_nonblocking, ec);

            if (ec) {
                CLUEAPI_LOG_WARNING(
                    "Failed to set acceptor nonblocking (val={}): {}",

                    acceptor_cfg.m_nonblocking, ec.message()
                );

                ec.clear();
            }

            if (acceptor_cfg.m_reuse_port) {
#if defined(SO_REUSEPORT)
                m_acceptor.set_option(
                    boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT>(true),

                    ec
                );

                if (ec) {
                    CLUEAPI_LOG_WARNING(
                        "Failed to set SO_REUSEPORT: {}",

                        ec.message()
                    );

                    ec.clear();
                }
#else
                CLUEAPI_LOG_DEBUG("SO_REUSEPORT not available on this platform");
#endif
            }

            if (acceptor_cfg.m_reuse_address) {
#if defined(SO_REUSEADDR)
                m_acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);

                if (ec) {
                    CLUEAPI_LOG_WARNING(
                        "Failed to set REUSEADDR: {}",

                        ec.message()
                    );

                    ec.clear();
                }
#else
                CLUEAPI_LOG_WARNING("SO_REUSEADDR not available on this platform.");
#endif
            }

            if (acceptor_cfg.m_tcp_fast_open) {
#if defined(TCP_FASTOPEN) && defined(IPPROTO_TCP)
                constexpr std::int32_t tfo_qlen = 5;

                const auto sock_opt_result
                    = setsockopt(m_acceptor.native_handle(), IPPROTO_TCP, TCP_FASTOPEN, &tfo_qlen, sizeof(tfo_qlen));

                if (sock_opt_result != 0) {
                    CLUEAPI_LOG_WARNING(
                        "Failed to set TCP_FASTOPEN: {}",

                        boost::system::error_code(errno, boost::asio::error::get_system_category()).message()
                    );
                }
#else
                CLUEAPI_LOG_WARNING("TCP_FASTOPEN not available or IPPROTO_TCP not defined.");
#endif
            }

            m_acceptor.bind(endpoint, ec);

            if (ec) {
                throw exceptions::exception_t(
                    "Failed to bind to {}:{}: {}",

                    host, m_port, ec.message()
                );
            }

            m_acceptor.listen(
                cfg.m_server.m_max_connections > 0 ? cfg.m_server.m_max_connections
                                                   : boost::asio::socket_base::max_listen_connections,

                ec
            );

            if (ec) {
                throw exceptions::exception_t(
                    "Failed to listen on {}:{}: {}",

                    host, m_port, ec.message()
                );
            }

            CLUEAPI_LOG_DEBUG(
                "Acceptor initialized and listening on {}:{}",

                host, m_port
            );
        }

      public:
        void start(std::uint32_t workers_count) {
            m_running.store(true, std::memory_order_release);

            const auto workers = workers_count > 0 ? workers_count : std::thread::hardware_concurrency();

            if (workers_count <= 0) {
                CLUEAPI_LOG_DEBUG(
                    "Overriding workers count to {} based on hardware concurrency",

                    workers
                );
            }

            auto acceptors_count = std::max(1u, workers / 6u);

            auto regular_workers = workers - acceptors_count;

            if (regular_workers < 1) {
                regular_workers = 1;

                acceptors_count = workers > 1 ? workers - 1 : 1;
            }

            CLUEAPI_LOG_DEBUG(
                "Spawning {} acceptor threads and {} regular worker threads",

                acceptors_count, regular_workers
            );

            {
                const auto max_connections = m_clueapi.cfg().m_server.m_max_connections;

                m_client_storage.reserve(max_connections);

                CLUEAPI_LOG_DEBUG("Pre-populating client pool with {} clients...", max_connections);

                for (std::size_t i{}; i < max_connections; i++) {
                    auto client = std::make_unique<client_t>(m_clueapi, *m_self);

                    return_client(client.get());

                    m_client_storage.push_back(std::move(client));
                }
            }

            m_self->set_keep_alive_timeout(fmt::format(
                "timeout={}",

                m_clueapi.cfg().m_http.m_keep_alive_timeout
            ));

            for (std::int32_t i{}; i < acceptors_count; ++i)
                boost::asio::co_spawn(m_io_ctx, do_accept(), boost::asio::detached);

            spawn_worker_threads(workers);
        }

        void stop() {
            if (!m_running.load(std::memory_order_acquire))
                return;

            m_running.store(false, std::memory_order_release);

            try {
                if (m_acceptor.is_open()) {
                    boost::system::error_code ec{};

                    m_acceptor.cancel(ec);

                    if (ec) {
                        CLUEAPI_LOG_ERROR(
                            "Failed to cancel acceptor: {}",

                            ec.message()
                        );

                        ec.clear();
                    }

                    m_acceptor.close(ec);

                    if (ec) {
                        CLUEAPI_LOG_ERROR(
                            "Failed to close acceptor: {}",

                            ec.message()
                        );

                        ec.clear();
                    }
                }

                m_client_storage.clear();

                m_io_ctx.stop();

                if (m_thread_pool) {
                    m_thread_pool->stop();
                    m_thread_pool->join();

                    m_thread_pool.reset();
                }

#if BOOST_VERSION <= 108600
                m_io_ctx.reset();
#endif
            }
            catch (const std::exception& e) {
                throw exceptions::exception_t(
                    "Exception during server stop: {}",

                    e.what()
                );
            }
            catch (...) {
                throw exceptions::exception_t("Unknown exception during server stop");
            }
        }

        shared::awaitable_t<void> do_accept() {
            if (!m_running.load(std::memory_order_acquire))
                co_return;

            const auto& cfg = m_clueapi.cfg();

            const auto executor = co_await boost::asio::this_coro::executor;

            while (m_running.load(std::memory_order_acquire) && m_acceptor.is_open()) {
                try {
                    boost::system::error_code ec{};

                    boost::asio::ip::tcp::socket socket(m_io_ctx);

                    co_await m_acceptor.async_accept(
                        socket,

                        boost::asio::redirect_error(boost::asio::use_awaitable, ec)
                    );

                    if (ec == boost::asio::error::operation_aborted || !m_running.load(std::memory_order_acquire)
                        || !m_acceptor.is_open())
                        break;

                    {
                        if (cfg.m_socket.m_tcp_no_delay) {
                            boost::asio::ip::tcp::no_delay no_delay_option(true);

                            socket.set_option(no_delay_option, ec);

#if defined(TCP_QUICKACK) && defined(__linux__)
                            constexpr std::int32_t quickack = 1;

                            auto res = setsockopt(
                                socket.native_handle(), IPPROTO_TCP, TCP_QUICKACK, &quickack, sizeof(quickack)
                            );

                            if (res != 0) {
                                CLUEAPI_LOG_WARNING(
                                    "Failed to set TCP_QUICKACK option: {}",

                                    boost::system::error_code(errno, boost::asio::error::get_system_category())
                                        .message()
                                );
                            }
#endif
                        }

                        if (cfg.m_socket.m_rcv_buf_size) {
                            boost::asio::socket_base::receive_buffer_size rcv_buf_size(cfg.m_socket.m_rcv_buf_size);

                            socket.set_option(rcv_buf_size, ec);

                            if (ec) {
                                CLUEAPI_LOG_WARNING(
                                    "Failed to set RCV_BUF_SIZE option: {}",

                                    ec.message()
                                );
                            }
                        }

                        if (cfg.m_socket.m_snd_buf_size) {
                            boost::asio::socket_base::send_buffer_size snd_buf_size(cfg.m_socket.m_snd_buf_size);

                            socket.set_option(snd_buf_size, ec);

                            if (ec) {
                                CLUEAPI_LOG_WARNING(
                                    "Failed to set SND_BUF_SIZE option: {}",

                                    ec.message()
                                );
                            }
                        }

                        if (cfg.m_socket.m_tcp_keep_alive) {
                            socket.set_option(boost::asio::socket_base::keep_alive(true), ec);

                            if (ec) {
                                CLUEAPI_LOG_WARNING(
                                    "Failed to enable TCP_KEEPALIVE: {}",

                                    ec.message()
                                );
                            }
                        }

                        if (ec) {
                            boost::system::error_code close_ec{};

                            socket.close(close_ec);

                            if (close_ec) {
                                CLUEAPI_LOG_WARNING(
                                    "Failed to close socket after error: {}",

                                    close_ec.message()
                                );
                            }

                            continue;
                        }
                    }

                    boost::asio::co_spawn(
                        executor,

                        [this, sock = std::move(socket)]() mutable -> shared::awaitable_t<void> {
                            auto client = get_client();

                            if (client) {
                                try {
                                    client->reset(std::move(sock));

                                    co_await client->start();
                                }
                                catch (...) {
                                    // ...
                                }

                                return_client(client);
                            }
                            else {
                                boost::system::error_code ec;

                                sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

                                sock.close(ec);
                            }
                        },

                        boost::asio::detached
                    );
                }
                catch (const std::exception& e) {
                    CLUEAPI_LOG_ERROR("Exception in acceptor: {}", e.what());
                }
                catch (...) {
                    CLUEAPI_LOG_ERROR("Unknown exception in acceptor");
                }
            }

            co_return;
        }

        client_t* get_client() {
            client_t* client_ptr{};

            if (m_client_pool.pop(client_ptr))
                return client_ptr;

            return nullptr;
        }

        void return_client(client_t* client) {
            if (!m_client_pool.push(client))
                CLUEAPI_LOG_WARNING("Failed to return client to the pool. Pool might be full.");
        }

        boost::asio::io_context& io_ctx() { return m_io_ctx; }

        bool is_running(std::memory_order m = std::memory_order_relaxed) const { return m_running.load(m); }

      private:
        void spawn_worker_threads(std::size_t workers) {
            m_thread_pool = std::make_unique<boost::asio::thread_pool>(workers);

            for (std::int32_t i{}; i < workers; i++) {
                boost::asio::post(*m_thread_pool, [this, i] {
                    try {
#ifdef __linux__
                        cpu_set_t cpuset;

                        CPU_ZERO(&cpuset);

                        CPU_SET(i % std::thread::hardware_concurrency(), &cpuset);

                        auto current_thread = pthread_self();

                        if (pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) != 0)
                            CLUEAPI_LOG_WARNING("Could not set CPU affinity for worker {}", i);
#endif // __linux__

                        m_io_ctx.run();
                    }
                    catch (const std::exception& e) {
                        CLUEAPI_LOG_ERROR(
                            "Exception in worker thread: {}",

                            e.what()
                        );
                    }
                    catch (...) {
                        CLUEAPI_LOG_ERROR("Unknown exception in worker thread");
                    }
                });
            }
        }

      public:
        c_server* m_self;

        c_clueapi& m_clueapi;

        boost::asio::io_context m_io_ctx;

        boost::asio::ip::tcp::acceptor m_acceptor;

        std::atomic_bool m_running;

        std::int32_t m_port;

        boost::lockfree::queue<client_t*> m_client_pool;

        std::vector<std::unique_ptr<client_t>> m_client_storage;

        std::unique_ptr<boost::asio::thread_pool> m_thread_pool;
    };

    c_server::c_server(c_clueapi& clueapi, const std::string& host, std::int32_t port)
        : m_impl(std::make_unique<c_impl>(clueapi, this, host, port)) {
        // ...
    }

    c_server::~c_server() {
        if (!m_impl || !m_impl->is_running())
            return;

        stop();
    }

    c_server::c_server(c_server&& other) noexcept
        : std::enable_shared_from_this<c_server>(std::move(other)), m_impl(std::move(other.m_impl)) {
        if (m_impl)
            m_impl->m_self = this;
    }

    c_server& c_server::operator=(c_server&& other) noexcept {
        if (this != &other) {
            try {
                if (m_impl && m_impl->is_running(std::memory_order_acquire)) {
                    CLUEAPI_LOG_DEBUG("[Server] Move assignment stopping current instance...");

                    stop();
                }
            }
            catch (...) {
                // ...
            }

            std::enable_shared_from_this<c_server>::operator=(std::move(other));

            m_impl = std::move(other.m_impl);

            if (m_impl)
                m_impl->m_self = this;
        }

        return *this;
    }

    void c_server::start(std::uint32_t workers_count) { m_impl->start(workers_count); }

    void c_server::stop() { m_impl->stop(); }

    boost::asio::io_context& c_server::io_ctx() { return m_impl->io_ctx(); }

    bool c_server::is_running(std::memory_order m) const { return m_impl->is_running(m); }

    void c_server::return_client_to_pool(client_t* client) { m_impl->return_client(client); }
}