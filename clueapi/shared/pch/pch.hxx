#ifndef CLUEAPI_SHARED_PCH_HXX
#define CLUEAPI_SHARED_PCH_HXX

#include <atomic>
#include <chrono>
#include <coroutine>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <tl/expected.hpp>

#include <ankerl/unordered_dense.h>

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/compile.h>
#include <fmt/core.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

#include <fmt/format.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <fmt/ranges.h>

#include <boost/asio.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/asio/this_coro.hpp>

#include <boost/asio/experimental/awaitable_operators.hpp>

#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/system/system_error.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <boost/filesystem.hpp>

#include <boost/lexical_cast.hpp>

#include <boost/lockfree/queue.hpp>

#ifdef CLUEAPI_USE_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif // CLUEAPI_USE_NLOHMANN_JSON

#ifdef CLUEAPI_USE_REDIS_MODULE
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/redis.hpp>
#endif // CXXAPI_USE_REDIS_IMPL

#endif // CLUEAPI_SHARED_PCH_HXX