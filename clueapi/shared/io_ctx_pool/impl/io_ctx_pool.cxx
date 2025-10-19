/**
 * @file io_ctx_pool.cxx
 *
 * @brief This file implements the `io_ctx_pool_t` struct, which manages a pool of
 * boost::asio::io_context objects.
 */

#include "clueapi/shared/io_ctx_pool/io_ctx_pool.hxx"

#include "clueapi/modules/macros.hxx"

#ifdef __linux__
#include <pthread.h>
#endif // __linux__

namespace clueapi::shared {
    void io_ctx_pool_t::start(std::size_t num_threads) {
        if (m_running.exchange(true) || num_threads == 0)
            return;

        {
            m_def_io_ctx = std::make_unique<io_ctx_t>();

            m_def_work_guard =
                std::make_unique<work_guard_t>(boost::asio::make_work_guard(*m_def_io_ctx));

            for (std::size_t i{}; i < num_threads; i++) {
                m_io_ctxs.emplace_back(std::make_unique<io_ctx_t>());

                m_work_guards.emplace_back(boost::asio::make_work_guard(*m_io_ctxs.back()));
            }
        }

        m_threads.reserve(num_threads + 1);

        {
            m_threads.emplace_back([this, num_threads] {
#ifdef __linux__
                cpu_set_t cpuset;

                CPU_ZERO(&cpuset);

                CPU_SET(num_threads % std::thread::hardware_concurrency(), &cpuset);

                auto current_thread = pthread_self();

                if (pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) != 0) {
                    CLUEAPI_LOG_WARNING("Could not set CPU affinity for worker {}", num_threads);
                }

                pthread_setname_np(current_thread, "def_io_ctx");

                {
                    auto* ctx_ptr = m_def_io_ctx.get();

                    {
                        __builtin_prefetch(ctx_ptr, 0, 3);

                        __builtin_prefetch(reinterpret_cast<char*>(ctx_ptr) + 64, 0, 3);

                        __builtin_prefetch(reinterpret_cast<char*>(ctx_ptr) + 128, 0, 3);

                        __builtin_prefetch(m_def_work_guard.get(), 0, 3);
                    }

                    volatile char cache_warmer{};

                    constexpr std::size_t k_cache_line_size = 64;
                    constexpr std::size_t k_max_warm_size = 512;

                    const auto warm_size = std::min(sizeof(*ctx_ptr), k_max_warm_size);

                    for (std::size_t offset{}; offset < warm_size; offset += k_cache_line_size)
                        cache_warmer += reinterpret_cast<volatile char*>(ctx_ptr)[offset];

                    (void)cache_warmer;
                }
#endif // __linux__

                m_def_io_ctx->run();
            });

            for (std::size_t i{}; i < num_threads; i++) {
                m_threads.emplace_back([this, i] {
#ifdef __linux__
                    cpu_set_t cpuset;

                    CPU_ZERO(&cpuset);

                    CPU_SET(i % std::thread::hardware_concurrency(), &cpuset);

                    auto current_thread = pthread_self();

                    if (pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) != 0)
                        CLUEAPI_LOG_WARNING("Could not set CPU affinity for worker {}", i);

                    pthread_setname_np(current_thread, fmt::format("io_worker_{}", i).c_str());

                    {
                        auto* ctx_ptr = m_io_ctxs[i].get();

                        {
                            __builtin_prefetch(ctx_ptr, 0, 3);

                            __builtin_prefetch(reinterpret_cast<char*>(ctx_ptr) + 64, 0, 3);

                            __builtin_prefetch(reinterpret_cast<char*>(ctx_ptr) + 128, 0, 3);

                            __builtin_prefetch(&m_work_guards[i], 0, 3);
                        }
                    }

                    {
                        volatile char cache_warmer{};

                        auto* ctx_ptr = m_io_ctxs[i].get();

                        constexpr std::size_t k_cache_line_size = 64;
                        constexpr std::size_t k_max_warm_size = 512;

                        const auto warm_size = std::min(sizeof(*ctx_ptr), k_max_warm_size);

                        for (std::size_t offset{}; offset < warm_size;
                             offset += k_cache_line_size) {
                            cache_warmer += reinterpret_cast<volatile char*>(ctx_ptr)[offset];
                        }

                        auto* guard_ptr = reinterpret_cast<volatile char*>(&m_work_guards[i]);

                        for (std::size_t offset{}; offset < sizeof(m_work_guards[i]);
                             offset += k_cache_line_size) {
                            cache_warmer += guard_ptr[offset];
                        }

                        (void)cache_warmer;
                    }
#endif // __linux__

                    m_io_ctxs[i]->run();
                });
            }
        }

        CLUEAPI_LOG_DEBUG("I/O context pool started with {} threads", num_threads + 1);
    }

    void io_ctx_pool_t::stop() {
        if (!m_running.exchange(false))
            return;

        m_work_guards.clear();

        if (m_def_work_guard) {
            m_def_work_guard->reset();

            m_def_work_guard.reset();
        }

        {
            for (auto& io_ctx : m_io_ctxs) {
                if (io_ctx && !io_ctx->stopped())
                    io_ctx->stop();
            }

            if (m_def_io_ctx && !m_def_io_ctx->stopped())
                m_def_io_ctx->stop();
        }

        const auto curr_thread_id = std::this_thread::get_id();

        for (auto& thread : m_threads) {
            if (!thread.joinable())
                continue;

            if (thread.get_id() == curr_thread_id) {
                thread.detach();
            } else {
                try {
                    thread.join();
                } catch (const std::exception& e) {
                    CLUEAPI_LOG_ERROR("Exception during thread join: {}", e.what());
                }
            }
        }

        m_threads.clear();

        m_io_ctxs.clear();

        m_def_io_ctx.reset();

        CLUEAPI_LOG_DEBUG("I/O context pool stopped");
    }
} // namespace clueapi::shared