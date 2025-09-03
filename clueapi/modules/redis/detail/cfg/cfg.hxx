#ifndef CLUEAPI_MODULES_REDIS_DETAIL_CFG_HXX
#define CLUEAPI_MODULES_REDIS_DETAIL_CFG_HXX

namespace clueapi::modules::redis::detail {
    struct cfg_t {
        std::string m_host{"127.0.0.1"};

        std::string m_port{"6379"};

        std::string m_password{};

        std::string m_username{};

        boost::redis::logger::level m_log_level{boost::redis::logger::level::info};
    };
} // namespace clueapi::modules::redis::detail

#endif // CLUEAPI_MODULES_REDIS_DETAIL_CFG_HXX