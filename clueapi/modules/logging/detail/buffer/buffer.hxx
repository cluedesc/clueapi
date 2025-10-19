/**
 * @file buffer.hxx
 *
 * @brief Defines a thread-safe buffer for log messages.
 */

#ifndef CLUEAPI_MODULES_LOGGING_DETAIL_BUFFER_HXX
#define CLUEAPI_MODULES_LOGGING_DETAIL_BUFFER_HXX

#ifdef CLUEAPI_USE_LOGGING_MODULE
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <vector>

#include "clueapi/modules/logging/detail/log/log.hxx"
#include "clueapi/shared/macros.hxx"

namespace clueapi::modules::logging::detail {
    /**
     * @struct msg_buffer_t
     *
     * @brief A thread-safe buffer for storing and batching log messages.
     *
     * @internal
     */
    struct msg_buffer_t {
        CLUEAPI_INLINE msg_buffer_t() noexcept : m_capacity{512} {
            m_buf.reserve(m_capacity);
        }

        CLUEAPI_INLINE msg_buffer_t(std::size_t capacity) noexcept : m_capacity{capacity} {
            m_buf.reserve(m_capacity);
        }

        CLUEAPI_INLINE ~msg_buffer_t() noexcept = default;

        CLUEAPI_INLINE msg_buffer_t(const msg_buffer_t&) noexcept = delete;

        CLUEAPI_INLINE msg_buffer_t& operator=(const msg_buffer_t&) noexcept = delete;

        CLUEAPI_INLINE msg_buffer_t(msg_buffer_t&& other) noexcept {
            std::unique_lock lock_other{other.m_mutex};

            m_buf = std::move(other.m_buf);

            m_capacity = other.m_capacity;
        }

        CLUEAPI_INLINE msg_buffer_t& operator=(msg_buffer_t&& other) noexcept {
            if (this != &other) {
                std::unique_lock lock{m_mutex};
                std::unique_lock lock_other{other.m_mutex};

                m_buf = std::move(other.m_buf);

                m_capacity = other.m_capacity;
            }

            return *this;
        }

       public:
        /**
         * @brief Pops a single message from the front of the buffer.
         *
         * @param msg A reference to store the popped message.
         *
         * @return `true` if a message was popped, `false` otherwise.
         */
        bool pop(log_msg_t& msg);

        /**
         * @brief Pushes a single message to the back of the buffer.
         *
         * @param msg The message to push.
         *
         * @return A pair containing a boolean indicating success and the original message if the
         * buffer was full.
         */
        std::pair<bool, log_msg_t> push(log_msg_t msg);

        /**
         * @brief Retrieves a batch of messages from the buffer.
         *
         * @param size The maximum number of messages to retrieve.
         *
         * @return A vector containing the batch of messages.
         */
        std::vector<log_msg_t> get_batch(std::size_t size);

       public:
        /**
         * @brief Destroys the buffer's contents, releasing memory.
         */
        CLUEAPI_INLINE void destroy() noexcept {
            std::unique_lock lock{m_mutex};

            std::vector<log_msg_t>().swap(m_buf);
        }

        /**
         * @brief Clears the buffer.
         */
        CLUEAPI_INLINE void clear() noexcept {
            m_buf.clear();
        }

       public:
        /**
         * @brief Sets the capacity of the buffer.
         *
         * @param capacity The new capacity.
         */
        CLUEAPI_INLINE void set_capacity(std::size_t capacity) noexcept {
            m_capacity = capacity;
        }

        /**
         * @brief Gets the capacity of the buffer.
         *
         * @return The capacity.
         */
        CLUEAPI_INLINE const auto& capacity() const noexcept {
            return m_capacity;
        }

        /**
         * @brief Gets the current size of the buffer.
         *
         * @return The number of messages in the buffer.
         */
        CLUEAPI_INLINE auto size() const noexcept {
            std::shared_lock<std::shared_mutex> lock{m_mutex};

            return m_buf.size();
        }

        /**
         * @brief Checks if the buffer is empty.
         *
         * @return `true` if the buffer is empty, `false` otherwise.
         */
        CLUEAPI_INLINE auto empty() const noexcept {
            std::shared_lock<std::shared_mutex> lock{m_mutex};

            return m_buf.empty();
        }

       private:
        /**
         * @brief The maximum capacity of the buffer.
         */
        std::size_t m_capacity{};

        /**
         * @brief The internal buffer.
         */
        std::vector<log_msg_t> m_buf;

        /**
         * @brief A mutex for thread-safe access to the buffer.
         */
        mutable std::shared_mutex m_mutex;
    };
} // namespace clueapi::modules::logging::detail
#endif // CLUEAPI_USE_LOGGING_MODULE

#endif // CLUEAPI_MODULES_LOGGING_DETAIL_BUFFER_HXX