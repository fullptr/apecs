#ifndef APECS_HPP_
#define APECS_HPP_

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <functional>
#include <initializer_list>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

namespace apx {
namespace meta {

template <typename T, typename Tuple>
struct tuple_contains;

template <typename T, typename... Ts>
struct tuple_contains<T, std::tuple<Ts...>> : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template <typename T, typename Tuple>
inline constexpr bool tuple_contains_v = tuple_contains<T, Tuple>::value;

template <typename Tuple, typename F>
constexpr void for_each(Tuple&& tuple, F&& f)
{
    std::apply([&](auto&&... x) { (f(x), ...); }, tuple);
}

template <typename T> struct tag
{
    static T type(); // Not implmented, to be used with decltype
};

template <typename... Ts>
struct get_first;

template <typename T, typename... Ts>
struct get_first<T, Ts...> { using type = T; };

}

template <typename T>
class sparse_set
{
public:
    using index_type = std::size_t;
    using value_type = T;

    using packed_type = std::vector<std::pair<index_type, value_type>>;
    using sparse_type = std::vector<index_type>;

private:
    static_assert(std::is_integral<index_type>());

    static constexpr index_type EMPTY = std::numeric_limits<index_type>::max();

    packed_type d_packed;
    sparse_type d_sparse;

    // Grows the sparse set so that the given index becomes valid.
    constexpr void assure(const index_type index)
    {
        assert(!has(index));
        if (d_sparse.size() <= index) {
            d_sparse.resize(index + 1, EMPTY);
        }
    }

public:
    constexpr sparse_set() noexcept = default;

    // Inserts the given value into the specified index. It is asserted that
    // no previous value exists at the index (see assert in assure()).
    constexpr value_type& insert(const index_type index, const value_type& value)
    {
        assure(index);
        d_sparse[index] = d_packed.size();
        return d_packed.emplace_back(index, value).second;
    }

    constexpr value_type& insert(const index_type index, value_type&& value)
    {
        assure(index);
        d_sparse[index] = d_packed.size();
        return d_packed.emplace_back(index, std::move(value)).second;
    }

    template <typename... Args>
    constexpr value_type& emplace(const index_type index, Args&&... args)
    {
        assure(index);
        d_sparse[index] = d_packed.size();
        return d_packed.emplace_back(std::piecewise_construct,
                                     std::forward_as_tuple(index),
                                     std::forward_as_tuple(std::forward<Args>(args)...)).second;
    }

    // Returns true if the specified index contains a value, and false otherwise.
    [[nodiscard]] bool has(const index_type index) const
    {
        return index < d_sparse.size() && d_sparse[index] != EMPTY;
    }

    // Removes all elements from the set.
    void clear() noexcept
    {
        d_packed.clear();
        d_sparse.clear();
    }

    // Removes the value at the specified index. The structure may reorder
    // itself to maintain contiguity for iteration.
    void erase(const index_type index)
    {
        assert(has(index));

        if (d_sparse[index] == d_packed.size() - 1) {
            d_sparse[index] = EMPTY;
            d_packed.pop_back();
            return;
        }

        // Pop the back element of the sparse_list
        auto back = d_packed.back();
        d_packed.pop_back();

        // Get the index of the outgoing value within the elements vector.
        const std::size_t packed_index = d_sparse[index];
        d_sparse[index] = EMPTY;

        // Overwrite the outgoing value with the back value.
        d_packed[packed_index] = back;

        // Point the index for the back value to its new location.
        d_sparse[back.first] = packed_index;
    }

    // Removes the value at the specified index, and does nothing if the index
    // does not exist. The structure may reorder itself to maintain element contiguity.
    void erase_if_exists(index_type index) noexcept
    {
        if (has(index)) {
            erase(index);
        }
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return d_packed.size();
    }

    [[nodiscard]] value_type& operator[](const index_type index)
    {
        assert(has(index));
        return d_packed[d_sparse[index]].second;
    }

    [[nodiscard]] const value_type& operator[](const index_type index) const
    {
        assert(has(index));
        return d_packed[d_sparse[index]].second;
    }

    [[nodiscard]] auto each() noexcept
    {
        return d_packed | std::views::transform([](auto& element) {
            return std::make_pair(std::cref(element.first), std::ref(element.second));
        });
    }

    [[nodiscard]] auto each() const noexcept
    {
        return d_packed | std::views::transform([](const auto& element) {
            return std::make_pair(std::cref(element.first), std::cref(element.second));
        });
    }
};

enum class entity : std::uint64_t {};
using index_t = std::uint32_t;
using version_t = std::uint32_t;

static constexpr apx::entity null{std::numeric_limits<std::uint64_t>::max()};

inline std::pair<index_t, version_t> split(const apx::entity id)
{
    using Int = std::underlying_type_t<apx::entity>;
    const Int id_int = static_cast<Int>(id);
    return {(index_t)(id_int >> 32), (version_t)id_int};
}

inline apx::entity combine(const index_t i, const version_t v)
{
    using Int = std::underlying_type_t<apx::entity>;
    return static_cast<apx::entity>(((Int)i << 32) + (Int)v);
}

inline apx::index_t to_index(const apx::entity entity)
{
    return apx::split(entity).first;
}

template <typename... Comps>
class handle;

template <typename... Comps>
class registry
{
public:
    template <typename T>
    using callback_t = std::function<void(apx::entity, const T&)>;

    using predicate_t = std::function<bool(apx::entity)>;

    // A tuple of tag types for metaprogramming purposes
    inline static constexpr std::tuple<apx::meta::tag<Comps>...> tags{};

    using handle_type = apx::handle<Comps...>;

private:
    using tuple_type = std::tuple<apx::sparse_set<Comps>...>;

    apx::sparse_set<apx::entity> d_entities;
    std::deque<apx::entity>      d_pool;

    tuple_type d_components;

    std::tuple<std::vector<callback_t<Comps>>...> d_on_add;
    std::tuple<std::vector<callback_t<Comps>>...> d_on_remove;

    template <typename Comp>
    void remove(const apx::entity entity, apx::sparse_set<Comp>& component_set)
    {
        if (has<Comp>(entity)) {
            for (auto& cb : std::get<std::vector<callback_t<Comp>>>(d_on_remove)) {
                cb(entity, get<Comp>(entity));
            }
            component_set.erase(apx::to_index(entity));
        }
    }

    template <typename Comp>
    [[nodiscard]] apx::sparse_set<Comp>& get_comps()
    {
        return std::get<apx::sparse_set<Comp>>(d_components);
    }

    template <typename Comp>
    [[nodiscard]] const apx::sparse_set<Comp>& get_comps() const
    {
        return std::get<apx::sparse_set<Comp>>(d_components);
    }

public:
    ~registry()
    {
        clear();
    }

    template <typename Comp>
    void on_add(callback_t<Comp>&& callback)
    {
        std::get<std::vector<callback_t<Comp>>>(d_on_add).push_back(std::move(callback));
    }

    template <typename Comp>
    void on_remove(callback_t<Comp>&& callback)
    {
        std::get<std::vector<callback_t<Comp>>>(d_on_remove).push_back(std::move(callback));
    }

    [[nodiscard]] apx::entity create()
    {
        index_t index = (index_t)d_entities.size();
        version_t version = 0;
        if (!d_pool.empty()) {
            std::tie(index, version) = split(d_pool.front());
            d_pool.pop_front();
            ++version;
        }

        const apx::entity id = combine(index, version);
        d_entities.insert(index, id);
        return id;
    }

    [[nodiscard]] bool valid(const apx::entity entity) const noexcept
    {
        const apx::index_t index = apx::to_index(entity);
        return entity != apx::null
            && d_entities.has(index)
            && d_entities[index] == entity;
    }

    void destroy(const apx::entity entity)
    {
        assert(valid(entity));
        remove_all_components(entity);
        d_pool.push_back(entity);
        d_entities.erase(apx::to_index(entity));
    }

    void destroy(const std::span<const apx::entity> entities)
    {
        std::ranges::for_each(entities, [&](auto e) { destroy(e); });
    }

    void destroy(const std::initializer_list<const apx::entity> entities)
    {
        std::ranges::for_each(entities, [&](auto e) { destroy(e); });
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return d_entities.size();
    }

    void clear()
    {
        for (auto [index, entity] : d_entities.each()) {
            remove_all_components(entity);
        }
        d_entities.clear();
        d_pool.clear();
    }

    template <typename Comp>
    Comp& add(const apx::entity entity, const Comp& component)
    {
        using T = std::remove_cvref_t<Comp>;
        static_assert(apx::meta::tuple_contains_v<apx::sparse_set<T>, tuple_type>);
        assert(valid(entity));

        auto& comp_set = get_comps<T>();
        auto& ret = comp_set.insert(apx::to_index(entity), component);
        for (auto cb : std::get<std::vector<callback_t<T>>>(d_on_add)) {
            cb(entity, ret);
        }
        return ret;
    }

    template <typename Comp>
    Comp& add(const apx::entity entity, Comp&& component)
    {
        using T = std::remove_cvref_t<Comp>;
        static_assert(apx::meta::tuple_contains_v<apx::sparse_set<T>, tuple_type>);
        assert(valid(entity));

        auto& comp_set = get_comps<T>();
        auto& ret = comp_set.insert(apx::to_index(entity), std::forward<Comp>(component));
        for (auto cb : std::get<std::vector<callback_t<T>>>(d_on_add)) {
            cb(entity, ret);
        }
        return ret;
    }

    template <typename Comp, typename... Args>
    Comp& emplace(const apx::entity entity, Args&&... args)
    {
        static_assert(apx::meta::tuple_contains_v<apx::sparse_set<Comp>, tuple_type>);
        assert(valid(entity));

        auto& comp_set = get_comps<Comp>();
        auto& ret = comp_set.emplace(apx::to_index(entity), std::forward<Args>(args)...);
        for (auto cb : std::get<std::vector<callback_t<Comp>>>(d_on_add)) {
            cb(entity, ret);
        }
        return ret;
    }

    template <typename Comp>
    void remove(const apx::entity entity)
    {
        static_assert(apx::meta::tuple_contains_v<apx::sparse_set<Comp>, tuple_type>);
        assert(valid(entity));

        auto& comp_set = get_comps<Comp>();
        return remove(entity, comp_set);
    }

    void remove_all_components(apx::entity entity)
    {
        apx::meta::for_each(d_components, [&](auto& comp_set) {
            remove(entity, comp_set);
        });
    }

    template <typename Comp>
    [[nodiscard]] bool has(const apx::entity entity) const noexcept
    {
        static_assert(apx::meta::tuple_contains_v<apx::sparse_set<Comp>, tuple_type>);
        assert(valid(entity));

        const auto& comp_set = get_comps<Comp>();
        return comp_set.has(apx::to_index(entity));
    }

    template <typename... Comps>
    [[nodiscard]] bool has_all(const apx::entity entity) const noexcept
    {
        assert(valid(entity));
        return (has<Comps>(entity) && ...);
    }

    template <typename... Comps>
    [[nodiscard]] bool has_any(const apx::entity entity) const noexcept
    {
        assert(valid(entity));
        return (has<Comps>(entity) || ...);
    }

    template <typename Comp>
    [[nodiscard]] Comp& get(const apx::entity entity) noexcept
    {
        static_assert(apx::meta::tuple_contains_v<apx::sparse_set<Comp>, tuple_type>);
        assert(has<Comp>(entity));

        auto& comp_set = get_comps<Comp>();
        return comp_set[apx::to_index(entity)];
    }

    template <typename Comp>
    [[nodiscard]] const Comp& get(const apx::entity entity) const noexcept
    {
        static_assert(apx::meta::tuple_contains_v<apx::sparse_set<Comp>, tuple_type>);
        assert(has<Comp>(entity));

        auto& comp_set = get_comps<Comp>();
        return comp_set[apx::to_index(entity)];
    }

    template <typename... Comps>
    [[nodiscard]] auto get_all(const apx::entity entity) noexcept
    {
        assert(has_all<Comps...>(entity));
        return std::make_tuple(std::ref(get<Comps>(entity))...);
    }

    template <typename... Comps>
    [[nodiscard]] auto get_all(const apx::entity entity) const noexcept
    {
        assert(has_all<Comps...>(entity));
        return std::make_tuple(std::cref(get<Comps>(entity))...);
    }

    template <typename Comp>
    [[nodiscard]] Comp* get_if(const apx::entity entity) noexcept
    {
        static_assert(apx::meta::tuple_contains_v<apx::sparse_set<Comp>, tuple_type>);
        return has<Comp>(entity) ? &get<Comp>(entity) : nullptr;
    }

    apx::entity from_index(std::size_t index) const noexcept
    {
        return d_entities[index];
    }

    [[nodiscard]] auto all() const noexcept
    {
        return d_entities.each() | std::views::values;
    }

    template <typename... Comps>
    [[nodiscard]] auto view() const noexcept
    {
        if constexpr (sizeof...(Comps) == 0) {
            return all();
        } else {
            using Comp = typename apx::meta::get_first<Comps...>::type;
            const auto entity_view = get_comps<Comp>().each() 
                | std::views::keys
                | std::views::transform([&](auto index) { return from_index(index); });

            if constexpr (sizeof...(Comps) > 1) {
                return entity_view | std::views::filter([&](auto entity) {
                    return has_all<Comps...>(entity);
                });
            } else {
                return entity_view;
            }
        }
    }

    template <typename... Comps> [[nodiscard]] auto view_get() noexcept
    {
        return view<Comps...>() | std::views::transform([&](auto entity) {
            return get_all<Comps...>(entity);
        });
    }

    template <typename... Comps> [[nodiscard]] auto view_get() const noexcept
    {
        return view<Comps...>() | std::views::transform([&](auto entity) {
            return get_all<Comps...>(entity);
        });
    }

    template <typename... Ts>
    void destroy_if(const predicate_t& cb) noexcept {
        auto v = view<Ts...>() | std::views::filter(cb);
        std::vector<apx::entity> to_delete{v.begin(), v.end()};
        destroy(to_delete);
    }

    template <typename... Comps>
    [[nodiscard]] apx::entity find(const predicate_t& predicate = [](apx::entity) { return true; }) const noexcept
    {
        auto v = view<Comps...>();
        if (auto result = std::ranges::find_if(v, predicate); result != v.end()) {
            return *result;
        }
        return apx::null;
    }
};

template <typename... Comps>
class handle
{
    apx::registry<Comps...>* d_registry;
    apx::entity              d_entity;

public:
    handle(apx::registry<Comps...>& r, const apx::entity e) : d_registry(&r), d_entity(e) {}
    explicit constexpr handle() : d_registry(nullptr), d_entity(apx::null) {}

    apx::entity entity() const { return d_entity; }

    [[nodiscard]] bool valid() noexcept { return d_registry->valid(d_entity); }
    void destroy() { d_registry->destroy(d_entity); }

    template <typename Comp>
    Comp& add(const Comp& component) { return d_registry->template add<Comp>(d_entity, component); }

    template <typename Comp>
    Comp& add(Comp&& component) { return d_registry->template add<Comp>(d_entity, std::forward<Comp>(component)); }

    template <typename Comp, typename... Args>
    Comp& emplace(Args&&... args) { return d_registry->template emplace<Comp>(d_entity, std::forward<Args>(args)...); }

    template <typename Comp>
    void remove() { d_registry->template remove<Comp>(d_entity); }

    void remove_all_components() { d_registry->remove_all_components(d_entity); }

    template <typename Comp>
    [[nodiscard]] bool has() const noexcept { return d_registry->template has<Comp>(d_entity); }

    template <typename... Comps>
    [[nodiscard]] bool has_all() const noexcept { return d_registry->template has_all<Comps...>(d_entity); }

    template <typename... Comps>
    [[nodiscard]] bool has_any() const noexcept { return d_registry->template has_any<Comps...>(d_entity); }

    template <typename Comp>
    [[nodiscard]] Comp& get() noexcept { return d_registry->template get<Comp>(d_entity); }

    template <typename Comp>
    [[nodiscard]] const Comp& get() const noexcept { return d_registry->template get<Comp>(d_entity); }

    template <typename Comp>
    [[nodiscard]] Comp* get_if() noexcept { return d_registry->template get_if<Comp>(d_entity); }

    apx::handle<Comps...>& operator=(const apx::handle<Comps...>& other) noexcept
    {
        d_registry = other.d_registry;
        d_entity = other.d_entity;
        return *this;
    }
    
    [[nodiscard]] bool operator==(const apx::handle<Comps...>& other) const noexcept
    {
        return d_registry == other.d_registry && d_entity == other.d_entity;
    }

    [[nodiscard]] bool operator!=(const apx::handle<Comps...>& other) const noexcept { return !(*this == other); }
};

template <typename... Comps>
inline apx::handle<Comps...> create_from(apx::registry<Comps...>& registry)
{
    return {registry, registry.create()};
}

template <typename... Comps>
static constexpr apx::handle null_handle{};

}

namespace std {

template <typename... Comps>
struct hash<apx::handle<Comps...>>
{
    std::size_t operator()(const apx::handle<Comps...>& handle) const {
        return std::hash<apx::entity>{}(handle.entity());
    }
};

}

#endif // APECS_HPP_
