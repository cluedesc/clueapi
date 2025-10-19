/**
 * @file logging.cxx
 *
 * @brief Implementation of the logging module.
 */

#include "clueapi/modules/logging/logging.hxx"

#ifdef CLUEAPI_USE_LOGGING_MODULE
namespace clueapi::modules::logging {
    void c_logging::init(cfg_t cfg) {
        m_cfg = cfg;

        m_is_running.store(true, std::memory_order_release);

        if (!m_cfg.m_async_mode)
            return;

        m_condition = std::make_shared<std::condition_variable_any>();

        m_thread = std::thread(&c_logging::process_async, this);
    }

    void c_logging::destroy() {
        if (!m_cfg.m_async_mode) {
            if (!m_loggers.empty())
                m_loggers.clear();

            return;
        }

        if (!m_is_running.exchange(false, std::memory_order_release))
            return;

        m_condition->notify_all();

        if (m_thread.joinable())
            m_thread.join();

        { /* process remaining logging messages */
            for (auto& [hash, logger] : m_loggers) {
                if (!logger->enabled() || !logger->async_mode())
                    continue;

                logger->process();
            }

            std::fflush(stdout);
        }

        m_loggers.clear();

        m_condition.reset();
    }

    void c_logging::add_logger(detail::hash_t hash, std::shared_ptr<base_logger_t> logger) {
        auto logger_final = std::move(logger);

        logger_final->set_prv_params(detail::prv_logger_params_t{.m_condition = m_condition});

        std::unique_lock<std::shared_mutex> lock(m_mutex);

        m_loggers.try_emplace(hash, std::move(logger_final));
    }

    void c_logging::remove_logger(detail::hash_t hash) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        if (!m_loggers.contains(hash))
            return;

        m_loggers.erase(hash);
    }

    std::shared_ptr<base_logger_t> c_logging::get_logger(detail::hash_t hash) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        if (!m_loggers.contains(hash))
            return nullptr;

        return m_loggers.at(hash);
    }

    void c_logging::process_async() {
        while (m_is_running.load(std::memory_order_relaxed)) {
            std::vector<std::shared_ptr<base_logger_t>> loggers;

            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);

                m_condition->wait_for(lock, m_cfg.m_sleep);

                if (!m_is_running.load(std::memory_order_relaxed))
                    break;

                for (auto& [hash, logger] : m_loggers) {
                    if (!logger->enabled() || !logger->async_mode())
                        continue;

                    loggers.push_back(logger);
                }

                if (loggers.empty())
                    continue;
            }

            for (auto& logger : loggers)
                logger->process();

            std::fflush(stdout);
        }
    }
} // namespace clueapi::modules::logging
#endif // CLUEAPI_USE_LOGGING_MODULE