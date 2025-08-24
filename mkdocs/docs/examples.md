# Examples

## Basic Usage

```cpp
#include <clueapi.hxx>

clueapi::http::types::response_t root_sync(clueapi::http::ctx_t ctx) {
    // first option for initializing the response
    clueapi::http::types::text_response_t resp{};

    {
        resp.body()   = "Hello, world!";
        resp.status() = clueapi::http::types::status_t::ok;
    }

    return resp;
}

clueapi::shared::awaitable_t<clueapi::http::types::response_t> root_async(clueapi::http::ctx_t ctx) {
    // second option for initializing the response
    co_return clueapi::http::types::text_response_t{
        "Hello, world!",
        clueapi::http::types::status_t::ok
    };
}

clueapi::http::types::response_t get_user(clueapi::http::ctx_t ctx) {
    std::vector<std::string> users{
        "user1",
        "user2",
        "user3",
    };

    clueapi::http::types::text_response_t resp{};

    // Extract the 'user_id' parameter from the URL
    const auto& user_id = std::stoi(ctx.params()["user_id"]);

    if (users.size() < user_id) {
        resp.body()   = "User not found";
        resp.status() = clueapi::http::types::status_t::not_found;

        return resp;
    }

    {
        resp.body()   = users.at(user_id);
        resp.status() = clueapi::http::types::status_t::ok;
    }

    return resp;
}

int main() {
    auto api = clueapi::api();

    auto cfg = clueapi::cfg_t{};

    {
        cfg.m_host = "localhost";
        cfg.m_port = "8080";

        cfg.m_server.m_workers = 4;
    }

    {
        api.add_method(
            clueapi::http::types::method_t::get,

            "/sync",

            root_sync
        );

        api.add_method(
            clueapi::http::types::method_t::get,

            "/async",

            root_async
        );

        api.add_method(
            clueapi::http::types::method_t::get,

            "/user/{user_id}",

            get_user
        );
    }

    api.start(cfg);

    { // block until the server is stopped
        api.wait();
    }

    api.stop();

    return 0;
}
```

## Middleware

```cpp
#include <clueapi.hxx>

// Define a middleware class
class c_tracing_middleware : public clueapi::middleware::c_base_middleware {
  public:
    // Override the handle method to implement the middleware logic
    clueapi::shared::awaitable_t<clueapi::http::types::response_t> handle(
        const clueapi::http::types::request_t& request,

        clueapi::middleware::next_t next
    ) override {
        CLUEAPI_LOG_INFO("Tracing middleware invoked");

        co_return co_await next(request);
    }
};

int
main() {
    auto api = clueapi::api();

    auto cfg = clueapi::cfg_t{};

    {
        cfg.m_host = "localhost";
        cfg.m_port = "8080";

        cfg.m_server.m_workers = 4;
    }

    { // Add the middleware to the middleware list
        api.add_middleware(std::make_shared<c_tracing_middleware>()); 
    }

    {
        api.add_method(
            clueapi::http::types::method_t::get,

            "/",

            [](clueapi::http::ctx_t ctx) -> clueapi::shared::awaitable_t<clueapi::http::types::response_t> {
                co_return clueapi::http::types::response_t{"Hello, world!"};
            }
        );
    }

    api.start(cfg);

    { // block until the server is stopped
        api.wait();
    }

    api.stop();

    return 0;
}
```