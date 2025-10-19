/**
 * @file io_ctx_pool.hxx
 *
 * @brief This file includes the `io_ctx_pool_t` struct, which manages a pool of
 * boost::asio::io_context objects.
 */

#ifndef CLUEAPI_SHARED_IO_CTX_POOL_HXX
#define CLUEAPI_SHARED_IO_CTX_POOL_HXX

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include "clueapi/shared/macros.hxx"

namespace clueapi::shared {
    /**
     * @struct io_ctx_pool_t
     *
     * @brief Manages a pool of boost::asio::io_context objects to distribute I/O workload across
     * multiple threads.
     *
     * @details This class is designed to create and manage a pool of `io_context` instances, each
     * running on its own thread. This is a common pattern for improving the performance of network
     * applications by parallelizing I/O operations. The pool also includes a "default" `io_context`
     * that can be used for specific, non-intensive tasks like accepting new connections.
     *
     * The pool uses a round-robin strategy to distribute work across the available contexts,
     * ensuring an even load distribution.
     */
    struct io_ctx_pool_t {
        /**
         * @brief Type alias for the Boost.Asio I/O context.
         */
        using io_ctx_t = boost::asio::io_context;

        /**
         * @brief Type alias for a unique pointer to an I/O context.
         */
        using io_ctx_ptr_t = std::unique_ptr<io_ctx_t>;

        /**
         * @brief Type alias for the work guard to keep the I/O context running.
         */
        using work_guard_t = boost::asio::executor_work_guard<io_ctx_t::executor_type>;

       public:
        /**
         * @brief Constructs an `io_ctx_pool_t` in a default, unstarted state.
         */
        CLUEAPI_INLINE io_ctx_pool_t() noexcept : m_running{false}, m_next_ctx{0} {
        }

       public:
        /**
         * @brief Starts the I/O context pool and its associated worker threads.
         *
         * @param num_threads The number of worker threads to create for the pool. This does not
         * include the thread for the default `io_context`.
         */
        void start(std::size_t num_threads = 1);

        /**
         * @brief Stops all I/O contexts and joins the worker threads.
         *
         * @details This method gracefully shuts down the pool by stopping all `io_context`
         * instances, resetting work guards, and joining all threads.
         */
        void stop();

       public:
        CLUEAPI_INLINE bool is_running() const noexcept {
            return m_running.load(std::memory_order_acquire);
        }

        /**
         * @brief Retrieves the default `io_context`.
         *
         * @return A raw pointer to the default `io_context`.
         */
        CLUEAPI_INLINE io_ctx_t* def_io_ctx() const noexcept {
            return m_def_io_ctx.get();
        }

        /**
         * @brief Retrieves the next `io_context` from the pool using a round-robin strategy.
         *
         * @return A raw pointer to a `io_ctx_t` from the pool.
         */
        CLUEAPI_INLINE io_ctx_t* io_ctx() const noexcept {
            const auto idx = m_next_ctx.fetch_add(1, std::memory_order_relaxed) % m_io_ctxs.size();

            return m_io_ctxs[idx].get();
        }

       private:
        /**
         * @brief The default I/O context, often used for some operations.
         */
        io_ctx_ptr_t m_def_io_ctx;

        /**
         * @brief The work guard for the default I/O context.
         */
        std::unique_ptr<work_guard_t> m_def_work_guard;

        /**
         * @brief The worker threads for the I/O contexts.
         */
        std::vector<std::thread> m_threads;

        /**
         * @brief The pool of `io_context` instances.
         */
        std::vector<io_ctx_ptr_t> m_io_ctxs;

        /**
         * @brief The work guards for the `io_context` pool.
         */
        std::vector<work_guard_t> m_work_guards;

        /**
         * @brief An atomic flag indicating if the pool is running.
         */
        std::atomic_bool m_running;

        /**
         * @brief An atomic counter for round-robin `io_context` selection.
         */
        mutable std::atomic_ulong m_next_ctx;
    };
} // namespace clueapi::shared

#endif // CLUEAPI_SHARED_IO_CTX_POOL_HXX