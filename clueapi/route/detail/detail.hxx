/**
 * @file detail.hxx
 *
 * @brief Defines the detail implementation of the radix tree for routing.
 */

#ifndef CLUEAPI_ROUTE_DETAIL_HXX
#define CLUEAPI_ROUTE_DETAIL_HXX

/**
 * @namespace clueapi::route::detail
 *
 * @brief Internal data definitions for the clueapi route handlers.
 *
 * @internal
 */
namespace clueapi::route::detail {
    /**
     * @brief A node in the radix tree.
     *
     * @tparam _type_t The type of the value.
     */
    template <typename _type_t>
    struct radix_node_t {
        /**
         * @brief Checks if the node is dynamic.
         *
         * @return `true` if the node is dynamic, `false` otherwise.
         */
        [[nodiscard]] CLUEAPI_INLINE bool is_dynamic() const noexcept {
            return !m_param_name.empty();
        }

       public:
        /**
         * @brief The values of the node.
         */
        shared::unordered_map_t<http::types::method_t::e_method, _type_t> m_values{};

        /**
         * @brief The children of the node.
         */
        shared::unordered_map_t<char, std::shared_ptr<radix_node_t<_type_t>>> m_children{};

        /**
         * @brief The dynamic child of the node.
         */
        std::shared_ptr<radix_node_t<_type_t>> m_dynamic_child{};

        /**
         * @brief The parameter name of the node.
         */
        std::string m_param_name;

        /**
         * @brief The prefix of the node.
         */
        std::string m_prefix;
    };

    /**
     * @brief A radix tree for routing.
     *
     * @tparam _type_t The type of the value.
     */
    template <typename _type_t>
    struct radix_tree_t {
        CLUEAPI_INLINE radix_tree_t() noexcept : m_root{std::make_shared<radix_node_t<_type_t>>()} {
        }

       public:
        /**
         * @brief Inserts a handler into the radix tree.
         *
         * @param method The HTTP method.
         * @param path The path.
         * @param handler The handler.
         */
        void insert(
            http::types::method_t::e_method method,

            http::types::path_t path,

            _type_t handler);

        /**
         * @brief Finds a handler in the radix tree.
         *
         * @param method The HTTP method.
         * @param path The path.
         *
         * @return The handler and the parameters.
         */
        std::optional<std::pair<_type_t, http::types::params_t>> find(
            http::types::method_t::e_method method,

            std::string_view path);

       public:
        /**
         * @brief Normalizes a path.
         *
         * @param path The path to normalize.
         *
         * @return The normalized path.
         */
        static http::types::path_t norm_path(const http::types::path_t& path) noexcept;

        /**
         * @brief Splits a path into segments.
         *
         * @param path The path to split.
         *
         * @return The path segments.
         */
        static std::vector<std::string_view> split_path_segments(std::string_view path) noexcept;

        /**
         * @brief Extracts the parameter name from a segment.
         *
         * @param segment The segment to extract the parameter name from.
         *
         * @return The parameter name.
         */
        static std::string_view extract_param_name(std::string_view segment) noexcept;

        /**
         * @brief Checks if a segment is dynamic.
         *
         * @param segment The segment to check.
         *
         * @return `true` if the segment is dynamic, `false` otherwise.
         */
        static bool is_dynamic_segment(std::string_view segment) noexcept;

        /**
         * @brief Checks if a segment is broken.
         *
         * @param segment The segment to check.
         *
         * @return `true` if the segment is broken, `false` otherwise.
         */
        static bool is_broken_segment(std::string_view segment) noexcept;

       private:
        std::shared_ptr<radix_node_t<_type_t>> m_root{};
    };

    /**
     * @brief A traits class for awaitable types.
     *
     * @tparam The type to check.
     */
    template <typename>
    struct awaitable_traits_t;

    /**
     * @brief A traits class for awaitable types.
     *
     * @tparam _type_t The type of the value.
     * @tparam _executor The type of the executor.
     */
    template <typename _type_t, typename _executor>
    struct awaitable_traits_t<shared::awaitable_t<_type_t, _executor>> {
        using value_type_t = _type_t;
    };

    /**
     * @brief A traits class for awaitable responses.
     *
     * @tparam The type to check.
     */
    template <typename>
    struct is_awaitable_response_t {
        static constexpr bool value = false;
    };

    /**
     * @brief A traits class for awaitable responses.
     *
     * @tparam _executor The type of the executor.
     */
    template <typename _executor>
    struct is_awaitable_response_t<shared::awaitable_t<http::types::base_response_t, _executor>> {
        static constexpr bool value = true;
    };

    /**
     * @brief A concept for awaitable handlers.
     *
     * @tparam _fn_t The type of the handler.
     */
    template <typename _fn_t>
    concept handler_awaitable_c = requires(_fn_t fn, http::ctx_t ctx) {
        { fn(std::move(ctx)) } -> std::same_as<shared::awaitable_t<http::types::base_response_t>>;
    } || is_awaitable_response_t<std::invoke_result_t<_fn_t, http::ctx_t>>::value;

    /**
     * @brief A concept for synchronous handlers.
     *
     * @tparam _fn_t The type of the handler.
     */
    template <typename _fn_t>
    concept handler_c = requires(_fn_t fn, http::ctx_t ctx) {
        { fn(std::move(ctx)) } -> std::same_as<http::types::base_response_t>;
    };
} // namespace clueapi::route::detail

#endif // CLUEAPI_ROUTE_DETAIL_HXX