#ifndef CLUEAPI_MODULES_REDIS_HXX
#define CLUEAPI_MODULES_REDIS_HXX

#include <memory>
#include <optional>

#include <boost/asio/io_context.hpp>

#include "clueapi/modules/redis/detail/detail.hxx"

#include "clueapi/shared/macros.hxx"

namespace clueapi::modules::redis {
    using cfg_t = detail::cfg_t;

    using connection_t = detail::connection_t;

    class c_redis {
       public:
        CLUEAPI_INLINE c_redis() = default;

        CLUEAPI_INLINE ~c_redis() {
            shutdown();
        }

       public:
        void init(cfg_t cfg, boost::asio::io_context* io_ctx = nullptr);

        void shutdown();

       public:
        std::shared_ptr<connection_t> create_connection(
            std::optional<connection_t::cfg_t> cfg = std::nullopt,
            boost::asio::io_context* io_ctx = nullptr);

       public:
        [[nodiscard]] bool is_running() const noexcept {
            return m_running;
        }

        [[nodiscard]] const auto& cfg() const noexcept {
            return m_cfg;
        }

        [[nodiscard]] boost::asio::io_context* io_ctx() const noexcept {
            return m_io_ctx;
        }

       private:
        bool m_running{false};

        cfg_t m_cfg{};

        boost::asio::io_context* m_io_ctx{nullptr};
    };
} // namespace clueapi::modules::redis

#endif // CLUEAPI_MODULES_REDIS_HXX