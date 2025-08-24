/**
 * @file detail.cxx
 *
 * @brief Implements the radix tree for routing.
 */

#include <clueapi.hxx>

namespace clueapi::route::detail {
    /**
     * @brief Computes the longest common prefix of two strings.
     *
     * @param a The first string.
     * @param b The second string.
     *
     * @return The length of the longest common prefix.
     */
    std::size_t longest_common_prefix(
        const std::string_view& a, const std::string_view& b) noexcept {
        std::size_t i{};

        const auto min_len = std::min(a.length(), b.length());

        while (i < min_len && a[i] == b[i])
            i++;

        return i;
    }

    template <typename _type_t>
    void radix_tree_t<_type_t>::insert(
        http::types::method_t::e_method method,

        http::types::path_t path,

        _type_t handler) {
        path = norm_path(path);

        auto current_node = m_root;

        std::string_view path_view{path};

        if (path_view == "/") {
            if (current_node->m_values.count(method)) {
                throw exceptions::exception_t("Handler for method at path '/' already exists.");
            }

            current_node->m_values.emplace(method, std::move(handler));

            return;
        }

        if (path_view.starts_with('/'))
            path_view.remove_prefix(1);

        while (true) {
            if (path_view.empty()) {
                if (current_node->m_values.count(method)) {
                    throw exceptions::exception_t(
                        "Handler for method at path '{}' already exists.",

                        path);
                }

                current_node->m_values.emplace(method, std::move(handler));

                return;
            }

            if (path_view.front() == '{') {
                const auto segments = split_path_segments(path_view);

                const auto segment = segments.front();

                if (is_broken_segment(segment)) {
                    throw exceptions::exception_t(
                        "Malformed dynamic segment: {}",

                        segment);
                }

                auto param_name = extract_param_name(segment);

                if (param_name.empty()) {
                    throw exceptions::exception_t(

                        "Dynamic segment without name in path: {}",

                        path);
                }

                if (current_node->m_dynamic_child && current_node->m_param_name != param_name) {
                    throw exceptions::exception_t(
                        "Ambiguous dynamic route at path: {}",

                        path);
                }

                if (!current_node->m_dynamic_child) {
                    current_node->m_dynamic_child = std::make_shared<radix_node_t<_type_t>>();

                    current_node->m_param_name = std::string{param_name};
                }

                current_node = current_node->m_dynamic_child;

                path_view.remove_prefix(segment.length());

                if (path_view.starts_with('/'))
                    path_view.remove_prefix(1);

                continue;
            }

            auto it = current_node->m_children.find(path_view.front());

            if (it == current_node->m_children.end()) {
                const auto dyn_pos = path_view.find('{');

                auto new_node = std::make_shared<radix_node_t<_type_t>>();

                if (dyn_pos == std::string_view::npos) {
                    new_node->m_prefix = std::string{path_view};

                    path_view = "";
                } else {
                    new_node->m_prefix = std::string{path_view.substr(0, dyn_pos)};

                    path_view.remove_prefix(dyn_pos);
                }

                current_node->m_children.emplace(new_node->m_prefix.front(), new_node);

                current_node = new_node;
            } else {
                auto child = it->second;

                const auto lcp = longest_common_prefix(path_view, child->m_prefix);

                if (lcp < child->m_prefix.length()) {
                    auto split_node = std::make_shared<radix_node_t<_type_t>>();
                    {
                        split_node->m_prefix = child->m_prefix.substr(lcp);
                        split_node->m_children = std::move(child->m_children);
                        split_node->m_values = std::move(child->m_values);
                        split_node->m_dynamic_child = std::move(child->m_dynamic_child);
                        split_node->m_param_name = std::move(child->m_param_name);
                    }

                    {
                        child->m_prefix.erase(lcp);
                        child->m_children = {{split_node->m_prefix.front(), split_node}};
                        child->m_values.clear();
                        child->m_dynamic_child = nullptr;
                        child->m_param_name.clear();
                    }
                }

                path_view.remove_prefix(lcp);

                current_node = child;
            }
        }
    }

    template <typename _type_t>
    std::optional<std::pair<_type_t, http::types::params_t>> radix_tree_t<_type_t>::find(
        http::types::method_t::e_method method,

        std::string_view path) {
        if (path.length() > 1 && path.back() == '/')
            path.remove_suffix(1);

        auto current_node = m_root;

        http::types::params_t params{};

        if (path == "/") {
            auto it = current_node->m_values.find(method);

            if (it != current_node->m_values.end())
                return std::make_pair(it->second, params);

            return std::nullopt;
        }

        if (path.starts_with('/'))
            path.remove_prefix(1);

        while (true) {
            if (path.empty()) {
                auto it = current_node->m_values.find(method);

                if (it != current_node->m_values.end())
                    return std::make_pair(it->second, params);

                return std::nullopt;
            }

            auto it = current_node->m_children.find(path.front());

            if (it != current_node->m_children.end()) {
                auto child = it->second;

                if (path.starts_with(child->m_prefix)) {
                    path.remove_prefix(child->m_prefix.length());

                    if (path.starts_with('/'))
                        path.remove_prefix(1);

                    current_node = child;

                    continue;
                }
            }

            if (current_node->m_dynamic_child) {
                const auto next_slash = path.find('/');

                auto param_value =
                    next_slash == std::string_view::npos ? path : path.substr(0, next_slash);

                if (!param_value.empty()) {
                    params.emplace(current_node->m_param_name, std::string{param_value});

                    path.remove_prefix(param_value.length());

                    if (path.starts_with('/'))
                        path.remove_prefix(1);

                    current_node = current_node->m_dynamic_child;

                    continue;
                }
            }

            return std::nullopt;
        }

        return std::nullopt;
    }

    template <typename _type_t>
    http::types::path_t radix_tree_t<_type_t>::norm_path(const http::types::path_t& path) noexcept {
        if (path.empty())
            return "/";

        if (path.size() > 1 && path.back() == '/')
            return path.substr(0, path.size() - 1);

        return path;
    }

    template <typename _type_t>
    std::vector<std::string_view> radix_tree_t<_type_t>::split_path_segments(
        std::string_view path) noexcept {
        std::vector<std::string_view> ret{};

        if (path.empty() || path == "/")
            return ret;

        if (path.starts_with('/'))
            path.remove_prefix(1);

        std::size_t start{};
        std::size_t end{};

        while ((end = path.find('/', start)) != std::string_view::npos) {
            ret.emplace_back(path.substr(start, end - start));

            start = end + 1;
        }

        if (start < path.length())
            ret.emplace_back(path.substr(start));

        return ret;
    }

    template <typename _type_t>
    std::string_view radix_tree_t<_type_t>::extract_param_name(std::string_view segment) noexcept {
        if (segment.size() < 3 || segment.front() != '{' || segment.back() != '}')
            return "";

        return segment.substr(1, segment.size() - 2);
    }

    template <typename _type_t>
    bool radix_tree_t<_type_t>::is_dynamic_segment(std::string_view segment) noexcept {
        return segment.size() > 2 && segment.front() == '{' && segment.back() == '}';
    }

    template <typename _type_t>
    bool radix_tree_t<_type_t>::is_broken_segment(std::string_view segment) noexcept {
        return (segment.front() == '{' && segment.back() != '}') ||
               (segment.front() != '{' && segment.back() == '}');
    }

    template struct radix_tree_t<std::shared_ptr<base_route_t>>;

    template struct radix_tree_t<std::function<http::types::response_t(http::ctx_t)>>;
} // namespace clueapi::route::detail