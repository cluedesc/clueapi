/**
 * @file clueapi.cxx
 *
 * @brief Implements the main application class `c_clueapi`.
 */

#include <clueapi.hxx>

namespace clueapi {
#ifdef CLUEAPI_USE_LOGGING_MODULE
    const std::unique_ptr<modules::logging::c_logging> g_logging = std::make_unique<modules::logging::c_logging>();
#endif // CLUEAPI_USE_LOGGING_MODULE

#ifdef CLUEAPI_USE_DOTENV_MODULE
    const std::unique_ptr<modules::dotenv::c_dotenv> g_dotenv = std::make_unique<modules::dotenv::c_dotenv>();
#endif // CLUEAPI_USE_DOTENV_MODULE

    class c_clueapi::c_impl {
      public:
        c_impl(c_clueapi* self) : m_self{self} {}

      public:
        void start(cfg_t cfg) {
            try {
                m_cfg = cfg;

#ifdef CLUEAPI_USE_LOGGING_MODULE
                modules::logging::cfg_t logging_cfg{};

                {
                    logging_cfg.m_async_mode    = m_cfg.m_logging_cfg.m_async_mode;
                    logging_cfg.m_default_level = m_cfg.m_logging_cfg.m_default_level;
                    logging_cfg.m_sleep         = m_cfg.m_logging_cfg.m_sleep;
                }

                g_logging->init(logging_cfg);

                auto logger = std::make_shared<modules::logging::console_logger_t>(modules::logging::logger_params_t{
                    .m_name       = m_cfg.m_logging_cfg.m_name,
                    .m_level      = m_cfg.m_logging_cfg.m_default_level,
                    .m_capacity   = m_cfg.m_logging_cfg.m_capacity,
                    .m_batch_size = m_cfg.m_logging_cfg.m_batch_size,
                    .m_async_mode = m_cfg.m_logging_cfg.m_async_mode
                });

                g_logging->add_logger(LOGGER_NAME("clueapi"), std::move(logger));
#endif // CLUEAPI_USE_LOGGING_MODULE

                if (m_cfg.m_host == "localhost")
                    m_cfg.m_host = "127.0.0.1";

                auto port = 8080u;

                auto [ptr, ec] = std::from_chars(
                    m_cfg.m_port.c_str(), m_cfg.m_port.c_str() + m_cfg.m_port.size(),

                    port
                );

                if (ec != std::errc{} || port <= 0) {
                    CLUEAPI_LOG_WARNING("Port number '{}' is not supported, using '8080' instead", m_cfg.m_port);

                    port = 8080;
                }

                m_cfg.m_port = std::to_string(port);

                if (!boost::filesystem::exists(m_cfg.m_server.m_tmp_dir)) {
                    boost::filesystem::create_directory(m_cfg.m_server.m_tmp_dir);

                    CLUEAPI_LOG_DEBUG("Created temporary directory: {}", m_cfg.m_server.m_tmp_dir);
                }

                init_middleware_chain();

                CLUEAPI_LOG_INFO("Trying to start server...");

                m_running.store(true, std::memory_order_release);

                m_server = std::make_shared<server::c_server>(*m_self, m_cfg.m_host, port);

                if (m_server) {
                    m_signals.emplace(m_server->io_ctx(), SIGINT, SIGTERM, SIGQUIT);

                    if (m_signals.has_value()) {
                        m_signals->async_wait([this](const boost::system::error_code& err_code, std::int32_t signo) {
                            if (!err_code)
                                this->stop();
                        });
                    }

                    return m_server->start(m_cfg.m_server.m_workers);
                }
            }
            catch (const std::exception& e) {
                CLUEAPI_LOG_CRITICAL(exceptions::exception_t::make(
                    "Failed to initialize clueapi: {}",

                    e.what()
                ));
            }
            catch (...) {
                CLUEAPI_LOG_CRITICAL(exceptions::exception_t::make("Failed to initialize clueapi"));
            }
        }

        void wait() {
            CLUEAPI_LOG_INFO(
                "Wait state initiated, thread blocked pending shutdown signal",

                m_cfg.m_host, m_cfg.m_port
            );

            {
                std::unique_lock lock(m_wait_mutex);

                m_wait_cv.wait(lock, [this] { return !m_running.load(std::memory_order_acquire); });
            }

            CLUEAPI_LOG_INFO(
                "Wait state terminated, shutdown procedure complete",

                m_cfg.m_host, m_cfg.m_port
            );
        }

        void stop() {
            try {
                if (!m_running.load(std::memory_order_acquire))
                    return;

                m_running.store(false, std::memory_order_seq_cst);
                
                { std::lock_guard lock(m_wait_mutex); }

                if (m_signals.has_value()) {
                    boost::system::error_code error_code{};

                    m_signals->cancel(error_code);

                    if (error_code) {
                        CLUEAPI_LOG_ERROR(
                            "Failed to cancel signals: {}",

                            m_cfg.m_host, m_cfg.m_port, error_code.message()
                        );
                    }
                }

                if (m_server) {
                    m_server->stop();
                    m_server.reset();

                    CLUEAPI_LOG_INFO("Trying to stop server...");
                }

                {
                    if (boost::filesystem::exists(m_cfg.m_server.m_tmp_dir)) {
                        if (!boost::filesystem::remove_all(m_cfg.m_server.m_tmp_dir)) {
                            CLUEAPI_LOG_ERROR(
                                "Failed to remove temporary directory: {}",

                                m_cfg.m_server.m_tmp_dir
                            );
                        }
                        else
                            CLUEAPI_LOG_DEBUG("Temporary directory removed: {}", m_cfg.m_server.m_tmp_dir);
                    }
                }

#ifdef CLUEAPI_USE_LOGGING_MODULE
                g_logging->destroy();
#endif // CLUEAPI_USE_LOGGING_MODULE

#ifdef CLUEAPI_USE_DOTENV_MODULE
                g_dotenv->destroy();
#endif // CLUEAPI_USE_DOTENV_MODULE
            }
            catch (const std::exception& e) {
                CLUEAPI_LOG_CRITICAL(exceptions::exception_t::make(
                    "Failed to destroy clueapi: {}",

                    e.what()
                ));
            }
            catch (...) {
                CLUEAPI_LOG_CRITICAL(exceptions::exception_t::make("Failed to destroy clueapi"));
            }

            m_wait_cv.notify_all();
        }

      public:
        const cfg_t& cfg() const { return m_cfg; }

        bool is_running(std::memory_order m = std::memory_order_acquire) const { return m_running.load(m); }

        void init_middleware_chain() {
            std::function<shared::awaitable_t<http::types::response_t>(const http::types::request_t&)> core
                = [this](const http::types::request_t& req) -> shared::awaitable_t<http::types::response_t> {
                auto opt_pair = m_routes.find(req.method(), req.uri());

                if (!opt_pair.has_value()) {
                    co_return make_error_response(
                        http::types::status_t::e_status::not_found,

                        http::types::status_t::to_str(404)
                    );
                }

                auto [route, params] = opt_pair.value();

                if (!route) {
                    co_return make_error_response(
                        http::types::status_t::e_status::not_found,

                        http::types::status_t::to_str(404)
                    );
                }

                auto ctx = co_await http::ctx_t::make_awaitable(
                    req, params,

                    m_cfg.m_http.m_multipart_parser_cfg
                );

                if (route->is_awaitable())
                    co_return co_await route->handle_awaitable(std::move(ctx));

                co_return route->handle(std::move(ctx));
            };

            auto chain = std::move(core);

            for (auto& middleware : std::ranges::reverse_view(m_middlewares)) {
                auto next_chain = std::move(chain);

                chain = [&middleware, next_chain = std::move(next_chain)](const http::types::request_t& req
                        ) -> shared::awaitable_t<http::types::response_t> {
                    co_return co_await middleware->handle(req, next_chain);
                };
            }

            if (!chain)
                throw exceptions::exception_t("Failed to initialize middleware chain");

            m_middlewares_chain = std::move(chain);
        }

        http::types::response_t make_error_response(http::types::status_t::e_status status, std::string_view message) {
            switch (m_cfg.m_http.m_def_response_class) {
                case http::types::e_response_class::json:
                    {
                        return http::types::response_class_t<http::types::json_response_t>::make(
                            http::types::json_t::json_obj_t{{"error", message}},

                            status
                        );

                        break;
                    }

                case http::types::e_response_class::plain:
                    {
                        return http::types::response_class_t<http::types::response_t>::make(
                            std::string{message},

                            status
                        );

                        break;
                    }

                default:
                    {
                        return http::types::response_class_t<http::types::response_t>::make(
                            std::string{message},

                            status
                        );

                        break;
                    }
            }
        }

        void add_route(
            http::types::method_t::e_method method, http::types::path_t path,
            std::function<shared::awaitable_t<http::types::response_t>(http::ctx_t)> async_handler
        ) {
            using route_handler_t
                = route::route_t<std::function<shared::awaitable_t<http::types::response_t>(http::ctx_t)>>;

            auto route = std::make_shared<route_handler_t>(method, path, std::move(async_handler));

            try {
                m_routes.insert(method, path, std::move(route));
            }
            catch (const std::exception& e) {
                CLUEAPI_LOG_ERROR(
                    "Failed to insert async route: {} {}: {}",

                    http::types::method_t::to_str(method), path, e.what()
                );
            }
        }

        void add_route(
            http::types::method_t::e_method method, http::types::path_t path,
            std::function<http::types::response_t(http::ctx_t)> sync_handler
        ) {
            using route_handler_t = route::route_t<std::function<http::types::response_t(http::ctx_t)>>;

            auto route = std::make_shared<route_handler_t>(method, path, std::move(sync_handler));

            try {
                m_routes.insert(method, path, std::move(route));
            }
            catch (const std::exception& e) {
                CLUEAPI_LOG_ERROR(
                    "Failed to insert sync route: {} {}: {}",

                    http::types::method_t::to_str(method), path, e.what()
                );
            }
        }

        void add_middleware(middleware::middleware_t middleware) { m_middlewares.push_back(std::move(middleware)); }

      public:
        shared::awaitable_t<bool> handle_request(http::types::request_t& request, http::types::response_t& response) {
            response.status() = http::types::status_t::e_status::unknown;

            try {
                if (m_middlewares_chain) {
                    response = co_await m_middlewares_chain(request);
                }
                else
                    CLUEAPI_LOG_ERROR("Middleware chain is not initialized");
            }
            catch (const std::exception& e) {
                CLUEAPI_LOG_ERROR(exceptions::exception_t::make(
                    "Failed to handle request: {}",

                    e.what()
                ));
            }
            catch (...) {
                CLUEAPI_LOG_ERROR(exceptions::exception_t::make("Failed to handle request: unknown"));
            }

            if (response.status() != http::types::status_t::e_status::unknown)
                co_return true;

            response = make_error_response(
                http::types::status_t::e_status::internal_server_error,

                http::types::status_t::to_str(500)
            );

            co_return false;
        }

      private:
        c_clueapi* m_self;

        cfg_t m_cfg;

        std::atomic_bool m_running;

        std::mutex m_wait_mutex;

        std::condition_variable m_wait_cv;

        std::shared_ptr<server::c_server> m_server;

        std::optional<boost::asio::signal_set> m_signals;

        std::vector<middleware::middleware_t> m_middlewares;

        route::detail::radix_tree_t<std::shared_ptr<route::base_route_t>> m_routes;

        middleware::next_t m_middlewares_chain;
    };

    c_clueapi::c_clueapi() : m_impl{std::make_unique<c_impl>(this)} {}

    c_clueapi::~c_clueapi() = default;

    void c_clueapi::start(cfg_t cfg) { m_impl->start(cfg); }

    void c_clueapi::wait() { m_impl->wait(); }

    void c_clueapi::stop() { m_impl->stop(); }

    shared::awaitable_t<bool>
    c_clueapi::handle_request(http::types::request_t& request, http::types::response_t& response) {
        co_return co_await m_impl->handle_request(request, response);
    }

    const cfg_t& c_clueapi::cfg() const { return m_impl->cfg(); }

    bool c_clueapi::is_running(std::memory_order m) const { return m_impl->is_running(m); }

    void c_clueapi::add_method(http::types::method_t::e_method method, http::types::path_t path, route_t&& handler) {
        std::visit(
            [&](auto&& curr_handler) {
                m_impl->add_route(
                    method,

                    path,

                    std::forward<decltype(curr_handler)>(curr_handler)
                );
            },

            std::move(handler)
        );
    }

    void c_clueapi::add_middleware(middleware::middleware_t middleware) {
        m_impl->add_middleware(std::move(middleware));
    }
}