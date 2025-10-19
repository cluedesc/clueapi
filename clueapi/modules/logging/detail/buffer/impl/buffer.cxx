/**
 * @file buffer.cxx
 *
 * @brief Implements the buffer class.
 */

#include "clueapi/modules/logging/detail/buffer/buffer.hxx"

#ifdef CLUEAPI_USE_LOGGING_MODULE
namespace clueapi::modules::logging::detail {
    bool msg_buffer_t::pop(log_msg_t& msg) {
        std::unique_lock<std::shared_mutex> lock{m_mutex};

        if (m_buf.empty())
            return false;

        msg = std::move(m_buf.front());

        m_buf.erase(m_buf.begin());

        return true;
    }

    std::pair<bool, log_msg_t> msg_buffer_t::push(log_msg_t msg) {
        std::unique_lock<std::shared_mutex> lock{m_mutex};

        if (m_buf.size() >= m_capacity)
            return std::make_pair(false, std::move(msg));

        m_buf.push_back(std::move(msg));

        return std::make_pair(true, log_msg_t{});
    }

    std::vector<log_msg_t> msg_buffer_t::get_batch(std::size_t size) {
        std::vector<log_msg_t> ret{};

        if (m_buf.empty())
            return ret;

        std::unique_lock<std::shared_mutex> lock{m_mutex};

        ret.reserve(std::min(m_buf.size(), size));

        for (std::size_t i{}; i < ret.capacity(); i++)
            ret.push_back(std::move(m_buf.at(i)));

        m_buf.erase(m_buf.begin(), m_buf.begin() + ret.size());

        return ret;
    }
} // namespace clueapi::modules::logging::detail
#endif // CLUEAPI_USE_LOGGING_MODULE