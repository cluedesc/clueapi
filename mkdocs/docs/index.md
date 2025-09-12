# Overview

**clueapi** is a modern C++20, high-performance web framework designed for building scalable, asynchronous web services and APIs. It is built from the ground up to leverage contemporary C++ features, most notably coroutines, to provide a clean, efficient, and highly concurrent programming model.

The framework's core philosophy is to offer developers fine-grained control and exceptional performance by using a non-blocking, asynchronous architecture powered by Boost.Asio. This makes `clueapi` an excellent choice for applications requiring low latency and the ability to handle a large number of simultaneous connections, such as real-time services, high-throughput APIs, and microservices.

## Key Features

* **Asynchronous to the Core**: Every I/O operation, from accepting connections to reading request bodies and writing responses, is built on a fully asynchronous model using C++20 coroutines. This avoids blocking threads and maximizes server throughput.

* **High-Performance Radix Tree Routing**: The framework employs a sophisticated prefix tree for request routing, which provides fast and flexible URL matching. It supports static paths, parameterized segments (e.g., `/users/{id}`), and correctly dispatches requests to different handlers based on the HTTP method. To prevent runtime issues, the router detects and reports ambiguous route definitions at startup.

* **Flexible Middleware Chain**: A powerful middleware system allows for the implementation of cross-cutting concerns. Each middleware component can inspect or modify the request before passing it to the next link in the chain. This provides a versatile mechanism for tasks like authentication, logging, and header manipulation. Middleware can also process the response after the main handler has executed or terminate the request early.

* **Advanced Multipart Streaming Parser**: The framework includes a robust `multipart/form-data` parser designed for performance and large file uploads. To conserve system memory, the parser can automatically spill file uploads to disk when they exceed a configurable in-memory size threshold, processing the data in chunks without loading the entire file into RAM.

* **Efficient Response Streaming**: `clueapi` provides utilities for sending large responses without buffering the entire content. It supports HTTP chunked transfer encoding for custom data streams and includes a specialized response type for streaming files directly from disk, automatically managing headers like `Content-Type` and `ETag`.

## Architecture

The architecture of `clueapi` is layered to provide a clear separation of concerns, from low-level connection management to high-level application logic.

1.  **Server and Connection Management**: The core server class initializes the network listener and manages a pool of worker threads. Upon accepting a new connection, it dispatches the socket to a connection handler object from a pre-allocated pool to manage the entire lifecycle of that client. This approach minimizes allocation overhead for new connections and promotes reuse. Each handler runs its own asynchronous loop, reading and writing data until the connection is closed.

2.  **Request Processing Pipeline**: An incoming HTTP request is first parsed into a structured object that provides easy access to its headers, body, and cookies. This request object is then passed into a user-defined middleware chain. If not handled by a middleware, the request reaches the router, which performs a lookup in the prefix tree to find the appropriate handler based on the request's path and method.

3.  **Context and Handlers**: The matched application handler receives a single context object encapsulating all request-specific data. This includes the request itself, any URL parameters extracted by the router, and parsed multipart form data. This design ensures that handlers have a clean and consolidated interface to all necessary information. Handlers can be simple synchronous functions or, for more complex I/O-bound tasks, asynchronous C++ coroutines.

4.  **Response Generation**: A handler's responsibility is to return a response object. The framework provides several built-in response types for common use cases like plain text, JSON, and file downloads. This response object then travels back up the middleware chain, allowing each middleware to inspect or modify it before it is finally serialized and sent back to the client by the connection handler.

## Roadmap

Future development is focused on enhancing the framework's enterprise-readiness and expanding its ecosystem.

* **OpenAPI Integration**: Introduce tooling to automatically generate OpenAPI (Swagger) specifications from route definitions. This will streamline API documentation, client library generation, and automated testing.

* **Database Abstraction Modules**: Develop dedicated, asynchronous modules for popular databases. This will include a fully asynchronous client for **PostgreSQL** with connection pooling to enable high-performance caching and messaging.

* **Metrics and Observability**: Create a module to expose application and performance metrics in the **Prometheus** format, allowing for seamless integration into modern monitoring, visualization, and alerting pipelines.

* **First-Class WebSocket Support**: Implement robust support for the WebSocket protocol to enable real-time, bidirectional communication for interactive applications like chat services and live data dashboards.