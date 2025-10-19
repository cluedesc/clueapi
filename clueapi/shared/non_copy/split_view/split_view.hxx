/**
 * @file split_view.hxx
 *
 * @brief Defines a `split_view_t` type for splitting a string view into a sequence of string
 * views without allocating new memory.
 *
 * @details This file provides a `split_view_t` struct that models a C++20 range,
 * allowing for efficient iteration over substrings separated by a delimiter.
 * It's particularly useful for "non-copy" operations as it avoids heap allocations.
 */

#ifndef CLUEAPI_SHARED_NON_COPY_SPLIT_VIEW_HXX
#define CLUEAPI_SHARED_NON_COPY_SPLIT_VIEW_HXX

#include <iterator>
#include <string_view>

#include "clueapi/shared/macros.hxx"

namespace clueapi::shared::non_copy {
    /**
     * @struct split_view_t
     *
     * @brief A range type that represents a sequence of string views from a larger string view.
     *
     * @details This view is created from an input string view and a delimiter. It provides
     * `begin()` and `end()` methods to create an iterator for traversing the substrings.
     */
    struct split_view_t {
        /**
         * @brief Constructs a `split_view_t` from an input string and a delimiter.
         *
         * @param input The string view to be split.
         * @param delimiter The string view to use as the delimiter.
         */
        CLUEAPI_INLINE constexpr split_view_t(
            std::string_view input, std::string_view delimiter) noexcept
            : m_input{input}, m_delimiter{delimiter} {
        }

       public:
        /**
         * @struct iterator_t
         *
         * @brief An iterator for the `split_view_t` range.
         *
         * @details This iterator is responsible for finding the next substring segment
         * based on the delimiter and holding a view of the current segment.
         */
        struct iterator_t {
            CLUEAPI_INLINE constexpr iterator_t() noexcept = default;

            /**
             * @brief Constructs an iterator with the input string and delimiter.
             *
             * @param input The string view being iterated.
             * @param delimiter The delimiter string view.
             */
            CLUEAPI_INLINE constexpr iterator_t(
                std::string_view input, std::string_view delimiter) noexcept
                : m_input{input}, m_delimiter{delimiter} {
            }

           public:
            /**
             * @brief Checks if the iterator has reached the end.
             *
             * @param unused A sentinel value indicating the end of the range.
             *
             * @return `true` if the iterator is not at the end, `false` otherwise.
             */
            CLUEAPI_INLINE constexpr bool operator!=(
                std::default_sentinel_t /* unused */) const noexcept {
                return m_pos != std::string_view::npos;
            }

            /**
             * @brief Dereferences the iterator to get the current substring.
             *
             * @return A string view of the current substring.
             */
            CLUEAPI_INLINE constexpr std::string_view operator*() const noexcept {
                return m_current;
            }

            /**
             * @brief Advances the iterator to the next substring.
             *
             * @return A reference to the advanced iterator.
             */
            CLUEAPI_INLINE constexpr iterator_t& operator++() noexcept {
                if (m_pos == std::string_view::npos)
                    return *this;

                auto next_pos = m_input.find(m_delimiter, m_pos + m_current.size());

                if (next_pos == std::string_view::npos) {
                    m_current = m_input.substr(m_pos + m_current.size());

                    m_pos = std::string_view::npos;
                } else {
                    m_pos = m_pos + m_current.size() + m_delimiter.size();

                    m_current = m_input.substr(m_pos, next_pos - m_pos);
                }

                return *this;
            }

           public:
            /**
             * @brief The position of the current substring within the input.
             */
            std::size_t m_pos{};

            /**
             * @brief The original input string view.
             */
            std::string_view m_input;

            /**
             * @brief The delimiter string view.
             */
            std::string_view m_delimiter;

            /**
             * @brief A string view of the current substring.
             */
            std::string_view m_current;
        };

       public:
        /**
         * @brief Returns an iterator to the beginning of the `split_view_t` range.
         *
         * @return An `iterator_t` pointing to the first substring.
         */
        [[nodiscard]] CLUEAPI_INLINE constexpr iterator_t begin() const noexcept {
            iterator_t it{m_input, m_delimiter};

            auto first_pos = m_input.find(m_delimiter);

            if (first_pos == std::string_view::npos) {
                it.m_current = m_input;

                it.m_pos = 0;
            } else {
                it.m_current = m_input.substr(0, first_pos);

                it.m_pos = 0;
            }

            return it;
        }

        /**
         * @brief Returns a sentinel to mark the end of the `split_view_t` range.
         *
         * @return A `std::default_sentinel_t` instance.
         */
        [[nodiscard]] CLUEAPI_INLINE constexpr std::default_sentinel_t end() const noexcept {
            return {};
        }

       public:
        /**
         * @brief The original input string view.
         */
        std::string_view m_input;

        /**
         * @brief The delimiter string view.
         */
        std::string_view m_delimiter;
    };

    /**
     * @brief A factory function to create a `split_view_t` instance.
     *
     * @param input The string view to be split.
     * @param delimiter The delimiter string view.
     *
     * @return A `split_view_t` instance.
     */
    CLUEAPI_INLINE constexpr split_view_t split(
        std::string_view input, std::string_view delimiter) noexcept {
        return split_view_t{input, delimiter};
    }
} // namespace clueapi::shared::non_copy

#endif // CLUEAPI_SHARED_NON_COPY_SPLIT_VIEW_HXX