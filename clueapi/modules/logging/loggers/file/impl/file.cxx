/**
 * @file file.cxx
 *
 * @brief Implementation of the file logger.
 */

#ifdef CLUEAPI_USE_LOGGING_MODULE
#include "clueapi/modules/logging/loggers/file/file.hxx"

#include "clueapi/modules/logging/detail/detail.hxx"

namespace clueapi::modules::logging {
    void file_logger_t::process() {
        file_path_changed();

        if (!m_file.is_open())
            return;

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

        detail::print(m_file, buffer);
    }

    void file_logger_t::handle_overflow(detail::log_msg_t msg) {
        if (m_buffer.empty()) {
            m_buffer.push(std::move(msg));

            return;
        }

        detail::log_msg_t old_msg{};

        if (!m_buffer.pop(old_msg))
            return;

        m_buffer.push(std::move(msg));

        m_prv_params.m_condition->notify_one();
    }

    void file_logger_t::log(detail::log_msg_t msg) {
        if (msg.m_level < m_params.m_level)
            return;

        if (m_params.m_async_mode) {
            auto push_pair = m_buffer.push(std::move(msg));

            if (!push_pair.first) {
                handle_overflow(std::move(push_pair.second));
            } else
                m_prv_params.m_condition->notify_one();
        } else {
            file_path_changed();

            if (!m_file.is_open())
                return;

            fmt::memory_buffer buffer{};

            if (detail::format(msg, buffer))
                detail::print(m_file, buffer);
        }
    }

    void file_logger_t::file_path_changed() {
        if (m_file_hash != m_prev_file_hash) {
            if (m_file.is_open())
                m_file.close();

            m_file.open(m_file_path, std::ios::out | std::ios::app);

            m_prev_file_hash = m_file_hash;
        }
    }
} // namespace clueapi::modules::logging
#endif // CLUEAPI_USE_LOGGING_MODULE