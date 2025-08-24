/**
 * @file console.cxx
 *
 * @brief Implementation of the console logger.
 */

#include <clueapi.hxx>

#ifdef CLUEAPI_USE_LOGGING_MODULE
namespace clueapi::modules::logging {
    void console_logger_t::process() {
        auto batch = m_buffer.get_batch(m_params.m_batch_size);

        if (batch.empty())
            return;

        detail::batch_memory_buffer_t buffer{};

        for (auto& msg : batch) {
            if (!detail::format(msg, buffer)) {
                buffer.clear();

                return;
            }
        }

        detail::print(buffer);
    }

    void console_logger_t::handle_overflow(log_msg_t msg) {
        if (m_buffer.empty()) {
            m_buffer.push(std::move(msg));

            return;
        }

        log_msg_t old_msg{};

        if (!m_buffer.pop(old_msg))
            return;

        m_buffer.push(std::move(msg));

        m_prv_params.m_condition->notify_one();
    }

    void console_logger_t::log(log_msg_t msg) {
        if (msg.m_level < m_params.m_level)
            return;

        if (m_params.m_async_mode) {
            auto push_pair = m_buffer.push(std::move(msg));

            if (!push_pair.first) {
                handle_overflow(std::move(push_pair.second));
            } else
                m_prv_params.m_condition->notify_one();
        } else {
            fmt::memory_buffer buffer{};

            if (detail::format(msg, buffer))
                detail::print(buffer);
        }
    }
} // namespace clueapi::modules::logging
#endif // CLUEAPI_USE_LOGGING_MODULE