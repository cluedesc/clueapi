/**
 * @file clueapi.cxx
 *
 * @brief Implements the main application class `c_clueapi`.
 */

#include <clueapi.hxx>

namespace clueapi {
#ifdef CLUEAPI_USE_LOGGING_MODULE
    const std::unique_ptr<modules::logging::c_logging> g_logging =
        std::make_unique<modules::logging::c_logging>();
#endif // CLUEAPI_USE_LOGGING_MODULE

#ifdef CLUEAPI_USE_DOTENV_MODULE
    const std::unique_ptr<modules::dotenv::c_dotenv> g_dotenv =
        std::make_unique<modules::dotenv::c_dotenv>();
#endif // CLUEAPI_USE_DOTENV_MODULE

    class c_clueapi::c_impl {
       public:
        /**
         * @brief The state of the clueapi instance.
         *
         * @details This struct represents the state of the clueapi instance.
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

            void update(e_state state) {
                m_state.store(state, std::memory_order_release);

                CLUEAPI_LOG_TRACE(
                    "Updated CLUEAPI state to '{}'",

                    state_t::to_str(state));
            }

            bool compare_exchange_strong(e_state expected, e_state desired) {
                auto success =
                    m_state.compare_exchange_strong(expected, desired, std::memory_order_acq_rel);

                if (success) {
                    CLUEAPI_LOG_TRACE(
                        "Updated and compared CLUEAPI state from '{}' to '{}'",

                        state_t::to_str(expected),
                        state_t::to_str(desired));
                }

                return success;
            }

            void log() const {
                CLUEAPI_LOG_TRACE("Current CLUEAPI state: '{}'", state_t::to_str(current()));
            }

            static void log(e_state state) {
                CLUEAPI_LOG_TRACE("Current CLUEAPI state: '{}'", state_t::to_str(state));
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

        c_impl(c_clueapi* self)
            : m_self{self}, m_state{state_t::stopped}, m_cfg{}, m_shutdown_requested{false} {
        }

        ~c_impl() noexcept {
            try {
                stop_sync();

                destroy_modules();
            } catch (...) {
                // ...
            }
        }

        void start(cfg_t cfg) {
            {
                std::unique_lock lock{m_state_mutex};

                CLUEAPI_LOG_TRACE("Trying to start application");

                auto current_state = m_state.current();

                if (current_state != state_t::stopped)
                    throw exceptions::exception_t{"Application is not in stopped state"};
            }

            m_cfg = std::move(cfg);

            try {
                init_modules();

                m_state.update(state_t::starting);

                {
                    m_io_ctx_pool.start(m_cfg.m_workers);

                    if (!m_io_ctx_pool.is_running())
                        throw exceptions::exception_t{"I/O context pool failed to start"};
                }

                auto* io_ctx = m_io_ctx_pool.def_io_ctx();

                if (!io_ctx)
                    throw exceptions::exception_t{"No I/O context available for signals"};

                {
                    CLUEAPI_LOG_TRACE(
                        "Setting up signal handlers for SIGINT, SIGTERM, SIGQUIT, SIGSEGV");

                    m_signals = std::make_unique<boost::asio::signal_set>(*io_ctx);

                    {
                        m_signals->add(SIGINT);

                        m_signals->add(SIGTERM);

#ifndef _WIN32
                        m_signals->add(SIGQUIT);
#endif // _WIN32

                        m_signals->add(SIGSEGV);
                    }

                    m_signals->async_wait([this](const boost::system::error_code& ec, int signo) {
                        if (ec == boost::asio::error::operation_aborted)
                            return;

                        CLUEAPI_LOG_DEBUG(
                            "Received signal {}, initiating graceful shutdown", signo);

                        if (m_shutdown_requested.exchange(true, std::memory_order_acq_rel))
                            return;

                        stop_async();
                    });
                }

                sanitize_cfg();

                create_tmp_dir();

                init_middleware_chain();

                {
                    m_server = std::make_unique<server::c_server>(
                        *m_self, m_io_ctx_pool, m_middleware_chain, m_cfg);

                    if (!m_server)
                        throw exceptions::exception_t{"Failed to initialize server"};

                    m_server->start();

                    if (!m_server->is_running())
                        throw exceptions::exception_t{"Server failed to start"};
                }

                m_state.update(state_t::running);
            } catch (const std::exception& e) {
                CLUEAPI_LOG_CRITICAL("Error during clueapi startup: {}", e.what());

                cleanup_on_error();

                m_state.update(state_t::stopped);

                throw;
            }

            m_start_cv.notify_all();
        }

        void wait() {
            {
                std::unique_lock lock{m_state_mutex};

                m_start_cv.wait(lock, [this] {
                    auto curr_state = m_state.current();

                    return curr_state == state_t::running || curr_state == state_t::stopped;
                });

                if (m_state.current() != state_t::running) {
                    CLUEAPI_LOG_TRACE("Wait aborted — application did not start");

                    return;
                }
            }

            {
                std::unique_lock lock{m_wait_mutex};

                CLUEAPI_LOG_INFO("Wait state initiated, thread blocked pending shutdown signal");

                m_wait_cv.wait(lock, [this] {
                    auto curr_state = m_state.current();

                    return curr_state == state_t::stopping || curr_state == state_t::stopped;
                });

                CLUEAPI_LOG_INFO("Wait state terminated, thread unblocked");
            }
        }

        void stop() {
            stop_sync();
        }

        const cfg_t& cfg() const noexcept {
            return m_cfg;
        }

        shared::io_ctx_pool_t& io_ctx_pool() noexcept {
            return m_io_ctx_pool;
        }

        [[nodiscard]] bool is_running() const noexcept {
            return m_state.current() == state_t::running;
        }

        [[nodiscard]] bool is_stopped() const noexcept {
            return m_state.current() == state_t::stopped;
        }

       public:
        void add_route(
            http::types::method_t::e_method method,
            http::types::path_t path,

            std::function<shared::awaitable_t<http::types::response_t>(http::ctx_t)>
                async_handler) {
            using route_handler_t = route::route_t<
                std::function<shared::awaitable_t<http::types::response_t>(http::ctx_t)>>;

            auto route = std::make_shared<route_handler_t>(method, path, std::move(async_handler));

            try {
                m_routes.insert(method, path, std::move(route));
            } catch (const std::exception& e) {
                CLUEAPI_LOG_ERROR(
                    "Failed to insert async route: {} {}: {}",

                    http::types::method_t::to_str(method),
                    path,
                    e.what());
            }
        }

        void add_route(
            http::types::method_t::e_method method,
            http::types::path_t path,

            std::function<http::types::response_t(http::ctx_t)> sync_handler) {
            using route_handler_t =
                route::route_t<std::function<http::types::response_t(http::ctx_t)>>;

            auto route = std::make_shared<route_handler_t>(method, path, std::move(sync_handler));

            try {
                m_routes.insert(method, path, std::move(route));
            } catch (const std::exception& e) {
                CLUEAPI_LOG_ERROR(
                    "Failed to insert sync route: {} {}: {}",

                    http::types::method_t::to_str(method),
                    path,
                    e.what());
            }
        }

        void add_middleware(middleware::middleware_t middleware) {
            m_middlewares.push_back(std::move(middleware));
        }

       private:
        void init_middleware_chain() {
            std::function<shared::awaitable_t<http::types::response_t>(
                const http::types::request_t&)>
                core = [this](const http::types::request_t& req)
                -> shared::awaitable_t<http::types::response_t> {
                auto opt_pair = m_routes.find(req.method(), req.uri());

                if (!opt_pair.has_value()) {
                    const auto status = http::types::status_t::not_found;

                    co_return make_error_response(status, http::types::status_t::to_str(status));
                }

                auto [route, params] = opt_pair.value();

                if (!route) {
                    const auto status = http::types::status_t::not_found;

                    co_return make_error_response(status, http::types::status_t::to_str(status));
                }

                auto ctx = co_await http::ctx_t::make_awaitable(
                    req,
                    params,

                    m_cfg.m_http.m_multipart_parser_cfg);

                if (route->is_awaitable())
                    co_return co_await route->handle_awaitable(std::move(ctx));

                co_return route->handle(std::move(ctx));
            };

            auto chain = std::move(core);

            for (auto& middleware : std::ranges::reverse_view(m_middlewares)) {
                auto next_chain = std::move(chain);

                chain = [&middleware,
                         next_chain = std::move(next_chain)](const http::types::request_t& req)
                    -> shared::awaitable_t<http::types::response_t> {
                    co_return co_await middleware->handle(req, next_chain);
                };
            }

            if (!chain)
                throw exceptions::exception_t("Failed to initialize middleware chain");

            m_middleware_chain = std::move(chain);
        }

        http::types::response_t make_error_response(
            http::types::status_t::e_status status, std::string_view message) const {
            switch (m_cfg.m_http.m_def_response_class) {
                case http::types::e_response_class::json: {
                    return http::types::response_class_t<http::types::json_response_t>::make(
                        http::types::json_t::json_obj_t{{"error", message}},

                        status);

                    break;
                }

                case http::types::e_response_class::plain: {
                    return http::types::response_class_t<http::types::response_t>::make(
                        std::string{message},

                        status);

                    break;
                }

                default: {
                    return http::types::response_class_t<http::types::response_t>::make(
                        std::string{message},

                        status);

                    break;
                }
            }
        }

        void init_modules() {
            CLUEAPI_LOG_TRACE("Initializing modules");

#ifdef CLUEAPI_USE_LOGGING_MODULE
            modules::logging::cfg_t logging_cfg{
                .m_async_mode = m_cfg.m_logging_cfg.m_async_mode,
                .m_sleep = m_cfg.m_logging_cfg.m_sleep,
                .m_default_level = m_cfg.m_logging_cfg.m_default_level};

            g_logging->init(logging_cfg);

            auto logger = std::make_shared<modules::logging::console_logger_t>(
                modules::logging::logger_params_t{
                    .m_name = m_cfg.m_logging_cfg.m_name,
                    .m_level = m_cfg.m_logging_cfg.m_default_level,
                    .m_capacity = m_cfg.m_logging_cfg.m_capacity,
                    .m_batch_size = m_cfg.m_logging_cfg.m_batch_size,
                    .m_async_mode = m_cfg.m_logging_cfg.m_async_mode});

            g_logging->add_logger(LOGGER_NAME("clueapi"), std::move(logger));
#endif

            CLUEAPI_LOG_TRACE("Modules initialized successfully");
        }

        void destroy_modules() {
#ifdef CLUEAPI_USE_LOGGING_MODULE
            if (g_logging) {
                try {
                    g_logging->destroy();
                } catch (...) {
                    // ...
                }
            }
#endif

#ifdef CLUEAPI_USE_DOTENV_MODULE
            if (g_dotenv) {
                try {
                    g_dotenv->destroy();
                } catch (...) {
                    // ...
                }
            }
#endif
        }

       private:
        void sanitize_cfg() {
            if (m_cfg.m_host == "localhost") {
                CLUEAPI_LOG_TRACE("Host set to 'localhost', changing to '127.0.0.1'");

                m_cfg.m_host = "127.0.0.1";
            }

            auto port = 8080u;

            auto [ptr, ec] = std::from_chars(
                m_cfg.m_port.c_str(),
                m_cfg.m_port.c_str() + m_cfg.m_port.size(),

                port);

            if (ec != std::errc{} || port == 0) {
                CLUEAPI_LOG_WARNING(
                    "Port number '{}' is not supported, using '8080' instead", m_cfg.m_port);

                port = 8080;
            }

            m_cfg.m_port = std::to_string(port);
        }

       private:
        void stop_async() {
            auto expected = state_t::running;

            if (!m_state.compare_exchange_strong(expected, state_t::stopping))
                return;

            auto shutdown_task = [this]() {
                CLUEAPI_LOG_TRACE("Starting graceful shutdown");

                cancel_signals();

                destroy_server();

                remove_tmp_dir();

                stop_io_ctx_pool();

                m_state.update(state_t::stopped);

                CLUEAPI_LOG_INFO("Graceful shutdown completed successfully");

                {
                    std::lock_guard lock{m_wait_mutex};

                    m_wait_cv.notify_all();
                }
            };

            try {
                std::thread{shutdown_task}.detach();
            } catch (const std::exception& e) {
                CLUEAPI_LOG_ERROR("Failed to start shutdown thread: {}", e.what());
            }
        }

        void stop_sync() {
            auto current_state = m_state.current();

            if (current_state == state_t::stopped || current_state == state_t::stopping)
                return;

            if (current_state == state_t::running)
                stop_async();

            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};

            while (m_state.current() != state_t::stopped) {
                if (std::chrono::steady_clock::now() > deadline) {
                    CLUEAPI_LOG_WARNING("Graceful shutdown timeout exceeded");

                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds{5});
            }

            if (m_state.current() == state_t::stopped)
                return;

            CLUEAPI_LOG_TRACE("Trying to stop application synchronously");

            cancel_signals();

            destroy_server();

            remove_tmp_dir();

            stop_io_ctx_pool();

            m_state.update(state_t::stopped);

            if (std::chrono::steady_clock::now() > deadline)
                CLUEAPI_LOG_ERROR("Synchronous shutdown timeout. Forced shutdown");
        }

        void cleanup_on_error() {
            CLUEAPI_LOG_DEBUG("Performing error cleanup");

            cancel_signals();

            destroy_server();

            remove_tmp_dir();

            stop_io_ctx_pool();
        }

        void cancel_signals() {
            if (!m_signals)
                return;

            CLUEAPI_LOG_TRACE("Cancelling signal handlers");

            boost::system::error_code ec{};

            m_signals->cancel(ec);

            if (ec)
                CLUEAPI_LOG_WARNING("Failed to cancel signal handlers: {}", ec.message());

            m_signals.reset();

            CLUEAPI_LOG_TRACE("Signal handlers cancelled");
        }

        void destroy_server() {
            if (!m_server)
                return;

            CLUEAPI_LOG_TRACE("Trying to destroy server");

            m_server->stop();

            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{3};

            while (m_server->is_running() && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds{10});

            m_server.reset();

            CLUEAPI_LOG_TRACE("Server destroyed");
        }

        void stop_io_ctx_pool() {
            try {
                m_io_ctx_pool.stop();
            } catch (const std::exception& e) {
                CLUEAPI_LOG_WARNING("Failed to stop I/O context pool: {}", e.what());
            }
        }

        void create_tmp_dir() {
            if (boost::filesystem::exists(m_cfg.m_server.m_tmp_dir))
                return;

            CLUEAPI_LOG_TRACE("Creating tmp directory '{}'", m_cfg.m_server.m_tmp_dir);

            auto created = boost::filesystem::create_directories(m_cfg.m_server.m_tmp_dir);

            if (!created)
                throw exceptions::exception_t{"Failed to create tmp directory"};

            CLUEAPI_LOG_TRACE("Tmp directory '{}' created", m_cfg.m_server.m_tmp_dir);
        }

        void remove_tmp_dir() {
            if (boost::filesystem::exists(m_cfg.m_server.m_tmp_dir)) {
                CLUEAPI_LOG_TRACE("Removing tmp directory '{}'", m_cfg.m_server.m_tmp_dir);

                auto count = boost::filesystem::remove_all(m_cfg.m_server.m_tmp_dir);

                CLUEAPI_LOG_TRACE(
                    "Removed {} files from tmp directory '{}'",

                    count,
                    m_cfg.m_server.m_tmp_dir);
            } else {
                CLUEAPI_LOG_TRACE(
                    "Can't remove tmp directory '{}' — it doesn't exist",

                    m_cfg.m_server.m_tmp_dir);
            }
        }

       private:
        c_clueapi* m_self;

        state_t m_state;

        cfg_t m_cfg;

        std::mutex m_state_mutex;

        std::condition_variable m_start_cv;

        std::atomic_bool m_shutdown_requested;

        std::mutex m_wait_mutex;

        std::condition_variable m_wait_cv;

        shared::io_ctx_pool_t m_io_ctx_pool;

        std::unique_ptr<boost::asio::signal_set> m_signals;

        std::unique_ptr<server::c_server> m_server;

        route::detail::radix_tree_t<std::shared_ptr<route::base_route_t>> m_routes;

        std::vector<middleware::middleware_t> m_middlewares;

        middleware::middleware_chain_t m_middleware_chain;
    };

    c_clueapi::c_clueapi() : m_impl{std::make_unique<c_impl>(this)} {
    }

    c_clueapi::~c_clueapi() noexcept = default;

    void c_clueapi::start(cfg_t cfg) {
        m_impl->start(std::move(cfg));
    }

    void c_clueapi::wait() {
        m_impl->wait();
    }

    void c_clueapi::stop() {
        m_impl->stop();
    }

    const cfg_t& c_clueapi::cfg() const noexcept {
        return m_impl->cfg();
    }

    shared::io_ctx_pool_t& c_clueapi::io_ctx_pool() const noexcept {
        return m_impl->io_ctx_pool();
    }

    bool c_clueapi::is_running() const noexcept {
        return m_impl->is_running();
    }

    bool c_clueapi::is_stopped() const noexcept {
        return m_impl->is_stopped();
    }

    void c_clueapi::add_method(
        http::types::method_t::e_method method, http::types::path_t path, route_t&& handler) {
        std::visit(
            [&](auto&& curr_handler) {
                m_impl->add_route(
                    method,

                    path,

                    std::forward<decltype(curr_handler)>(curr_handler));
            },

            std::move(handler));
    }

    void c_clueapi::add_middleware(middleware::middleware_t middleware) {
        m_impl->add_middleware(std::move(middleware));
    }

    void c_clueapi::enable_default_handlers() {
        CLUEAPI_LOG_DEBUG("Enabling default handlers");

        add_method(
            http::types::method_t::get,

            "/favicon.ico",

            [](http::ctx_t ctx) -> shared::awaitable_t<http::types::response_t> {
                co_return http::types::response_t{"", http::types::status_t::no_content};
            });

        add_method(
            http::types::method_t::get,

            "/robots.txt",

            [](http::ctx_t ctx) -> shared::awaitable_t<http::types::response_t> {
                co_return http::types::response_t{"", http::types::status_t::ok};
            });

        add_method(
            http::types::method_t::get,

            "/.well-known/appspecific/com.chrome.devtools.json",

            [](http::ctx_t ctx) -> shared::awaitable_t<http::types::response_t> {
                co_return http::types::json_response_t{
                    http::types::json_t::json_obj_t{}, http::types::status_t::ok};
            });
    }
} // namespace clueapi