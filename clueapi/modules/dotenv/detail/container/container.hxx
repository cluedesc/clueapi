/**
 * @file container.hxx
 *
 * @brief Defines a generic container for the dotenv module.
 */

#ifndef CLUEAPI_MODULES_DOTENV_DETAIL_CONTAINER_HXX
#define CLUEAPI_MODULES_DOTENV_DETAIL_CONTAINER_HXX

#ifdef CLUEAPI_USE_DOTENV_MODULE
namespace clueapi::modules::dotenv::detail {
    /**
     * @struct container_t
     *
     * @brief A wrapper around an unordered map to store key-value pairs.
     *
     * @tparam _key_t The type for the keys.
     * @tparam _value_t The type for the values.
     *
     * @internal
     */
    template <typename _key_t, typename _value_t>
    struct container_t {
        CLUEAPI_INLINE constexpr container_t() noexcept = default;

       public:
        /**
         * @brief Swaps the internal map with an empty one to release memory.
         */
        CLUEAPI_INLINE void destroy() noexcept {
            shared::unordered_map<_key_t, _value_t>().swap(m_container);
        }

       public:
        CLUEAPI_INLINE _value_t& operator[](const _key_t& key) noexcept {
            return m_container.at(key);
        }

        CLUEAPI_INLINE const _value_t& operator[](const _key_t& key) const noexcept {
            return m_container.at(key);
        }

       public:
        /**
         * @brief Retrieves a mutable reference to the value for a given key.
         *
         * @param key The key of the element to access.
         *
         * @return A mutable reference to the value.
         *
         * @throws std::out_of_range if the key does not exist.
         */
        CLUEAPI_INLINE _value_t& at(const _key_t& key) {
            return m_container.at(key);
        }

        /**
         * @brief Retrieves a const reference to the value for a given key.
         *
         * @param key The key of the element to access.
         *
         * @return A const reference to the value.
         *
         * @throws std::out_of_range if the key does not exist.
         */
        [[nodiscard]] CLUEAPI_INLINE const _value_t& at(const _key_t& key) const {
            return m_container.at(key);
        }

        /**
         * @brief Inserts or updates a key-value pair.
         *
         * @param key The key of the element to set.
         * @param value The value to associate with the key.
         */
        CLUEAPI_INLINE void set(const _key_t& key, const _value_t& value) {
            m_container.insert_or_assign(key, value);
        }

        /**
         * @brief Checks if an element with a specific key exists.
         *
         * @param key The key to check for.
         *
         * @return `true` if an element with the given key exists, `false` otherwise.
         */
        [[nodiscard]] CLUEAPI_INLINE bool contains(const _key_t& key) const noexcept {
            return m_container.contains(key);
        }

        /**
         * @brief Reserves space in the underlying map for a number of elements.
         *
         * @param size The minimum number of elements to reserve space for.
         */
        CLUEAPI_INLINE void reserve(std::size_t size) noexcept {
            m_container.reserve(size);
        }

        /**
         * @brief Erases an element with a specific key.
         *
         * @param key The key of the element to erase.
         */
        CLUEAPI_INLINE void erase(const _key_t& key) noexcept {
            m_container.erase(key);
        }

        /**
         * @brief Clears the container, removing all key-value pairs.
         */
        CLUEAPI_INLINE void clear() noexcept {
            m_container.clear();
        }

        /**
         * @brief Gets the number of elements in the container.
         *
         * @return The number of key-value pairs.
         */
        [[nodiscard]] CLUEAPI_INLINE auto size() const noexcept {
            return m_container.size();
        }

       private:
        /**
         * @brief The internal container.
         */
        shared::unordered_map<_key_t, _value_t> m_container{};
    };
} // namespace clueapi::modules::dotenv::detail
#endif // CLUEAPI_USE_DOTENV_MODULE

#endif // CLUEAPI_MODULES_DOTENV_DETAIL_CONTAINER_HXX