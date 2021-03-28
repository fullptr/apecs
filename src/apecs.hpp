#include <deque>
#include <tuple>
#include <utility>
#include <vector>
#include <cstdint>
#include <cassert>

#include <fmt/format.h>
#include <cppcoro/generator.hpp>

namespace apx {
namespace meta {

template <typename T, typename Tuple>
struct tuple_contains;

template <typename T, typename U, typename... Ts>
struct tuple_contains<T, std::tuple<U, Ts...>> : tuple_contains<T, std::tuple<Ts...>> {};

template <typename T, typename... Ts>
struct tuple_contains<T, std::tuple<T, Ts...>> : std::true_type {};

template <typename T>
struct tuple_contains<T, std::tuple<>> : std::false_type {};

template <typename T, typename Tuple>
using tuple_contains_v = tuple_contains<T, Tuple>::value;

template <class Tuple, class F>
constexpr void for_each(Tuple&& tuple, F&& f)
{
    [] <std::size_t... I> (Tuple&& tuple, F&& f, std::index_sequence<I...>)
    {
        (f(std::get<I>(tuple)), ...);
    }(
        std::forward<Tuple>(tuple),
        std::forward<F>(f),
        std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{}
    );
}

}

template <typename value_type>
class sparse_set
{
public:
    using index_type = std::size_t;

    using packed_type = std::vector<std::pair<index_type, value_type>>;
    using sparse_type = std::vector<index_type>;

    using iterator = typename packed_type::iterator;

private:
    static_assert(std::is_default_constructible<value_type>());
    static_assert(std::is_copy_constructible<value_type>());
    static_assert(std::is_integral<index_type>());

    static constexpr index_type EMPTY = std::numeric_limits<index_type>::max();

    packed_type d_packed;
    sparse_type d_sparse;

    // Grows the sparse set so that the given index becomes valid.
    void assure(index_type index);

public:
    sparse_set() = default;

    // Inserts the given value into the specified index. If a value already
    // exists at that index, it is overwritten.
    value_type& insert(index_type index, const value_type& value);

    // Returns true if the specified index contains a value, and false otherwise.
    bool has(index_type index) const;

    // Removes all elements from the set.
    void clear();

    // Removes the value at the specified index. The structure may reorder
    // itself to maintain contiguity for iteration.
    void erase(index_type index);

    // Removes the value at the specified index, and does nothing if the index
    // does not exist. The structure may reorder itself to maintain element contiguity.
    void erase_if_exists(index_type index) noexcept;

    std::size_t size() const;

    value_type& operator[](index_type index);
    const value_type& operator[](index_type index) const;

    // Provides a generator that loops over the packed set, which is fast but
    // results in undefined behaviour when removing elements.
    cppcoro::generator<std::pair<const index_type, value_type>&> fast();

    // Provides a generator that loops over the sparse set, which is slow but
    // allows for deleting elements while looping.
    cppcoro::generator<std::pair<const index_type, value_type>&> safe();
};


template <typename value_type>
void sparse_set<value_type>::assure(index_type index) {
    if (d_sparse.size() <= index) {
        d_sparse.resize(index + 1, EMPTY);
    }
}

template <typename value_type>
value_type& sparse_set<value_type>::insert(index_type index, const value_type& value)
{
    assert(!has(index));
    assure(index);
    index_type location = d_packed.size();
    d_sparse[index] = location;
    return d_packed.emplace_back(std::make_pair(index, value)).second;
}

template <typename value_type>
bool sparse_set<value_type>::has(index_type index) const
{
    return index < d_sparse.size() && d_sparse[index] != EMPTY;
}

template <typename value_type>
void sparse_set<value_type>::clear()
{
    d_packed.clear();
    d_sparse.clear();
}

template <typename value_type>
void sparse_set<value_type>::erase(index_type index)
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
    std::size_t packed_index = d_sparse[index];
    d_sparse[index] = EMPTY;

    // Overwrite the outgoing value with the back value.
    d_packed[packed_index] = back;

    // Point the index for the back value to its new location.
    d_sparse[back.first] = packed_index;
}

template <typename value_type>
void sparse_set<value_type>::erase_if_exists(index_type index) noexcept
{
    if (has(index)) {
        erase(index);
    }
}

template <typename value_type>
std::size_t sparse_set<value_type>::size() const
{
    return d_packed.size();
}

template <typename value_type>
value_type& sparse_set<value_type>::operator[](index_type index)
{
    if (has(index)) {
        return d_packed[d_sparse[index]].second;
    }
    return insert(index, value_type{});
}

template <typename value_type>
const value_type& sparse_set<value_type>::operator[](index_type index) const
{
    assert(has(index));
    return d_packed[d_sparse[index]].second;
}

template <typename value_type>
auto sparse_set<value_type>::fast() -> cppcoro::generator<std::pair<const index_type, value_type>&>
{
    for (auto pair : d_packed) {
        co_yield pair;
    }
}

template <typename value_type>
auto sparse_set<value_type>::safe() -> cppcoro::generator<std::pair<const index_type, value_type>&>
{
    for (auto index : d_sparse) {
        if (index != EMPTY) {
            co_yield d_packed[index];
        }
    }
}

enum class entity : std::uint64_t {};
using index_t = std::uint32_t;
using version_t = std::uint32_t;

static const apx::entity null{std::numeric_limits<std::uint64_t>::max()};

inline std::pair<index_t, version_t> split(apx::entity id)
{
    using Int = std::underlying_type_t<apx::entity>;
    Int id_int = static_cast<Int>(id);
    return {(index_t)(id_int >> 32), (version_t)id_int};
}

inline apx::entity combine(index_t i, version_t v)
{
    using Int = std::underlying_type_t<apx::entity>;
    return static_cast<apx::entity>(((Int)i << 32) + (Int)v);
}

inline apx::index_t to_index(apx::entity entity)
{
    return apx::split(entity).first;
}

template <typename... Comps>
class registry
{
    using tuple_type = std::tuple<apx::sparse_set<Comps>...>;

    apx::sparse_set<apx::entity> d_entities;
    std::deque<apx::entity>      d_pool;

    tuple_type d_components;

    template <typename Comp>
    void remove(apx::entity entity, apx::sparse_set<Comp>& component_set)
    {
        fmt::print("Removing {}\n", __FUNCSIG__);
        component_set.erase_if_exists(apx::to_index(entity));
    }

    template <typename Comp>
    using tuple_contains_v = apx::meta::tuple_contains<apx::sparse_set<Comp>, tuple_type>::value;

public:
    ~registry()
    {
        clear();
    }

    apx::entity create()
    {
        index_t index = d_entities.size();
        version_t version = 0;
        if (!d_pool.empty()) {
            std::tie(index, version) = split(d_pool.front());
            d_pool.pop_front();
            ++version;
        }

        apx::entity id = combine(index, version);
        d_entities.insert(index, id);
        return id;
    }

    bool valid(apx::entity entity) noexcept
    {
        apx::index_t index = apx::to_index(entity);
        return entity != apx::null
            && d_entities.has(index)
            && d_entities[index] == entity;
    }

    void destroy(apx::entity entity)
    {
        assert(valid(entity));
        apx::meta::for_each(d_components, [&](auto& comp_set) {
            remove(entity, comp_set);
        });

        d_pool.push_back(entity);
        d_entities.erase(apx::to_index(entity));
    }

    void clear()
    {
        for (auto [idx, entity] : d_entities.safe()) {
            destroy(entity);
        }
        d_entities.clear();
        d_pool.clear();
        std::swap(d_components, tuple_type{});
    }

    template <typename Comp>
    Comp& add(apx::entity entity, const Comp& component)
    {
        static_assert(tuple_contains_v<Comp>);
        assert(valid(entity));

        auto& comp_set = std::get<apx::sparse_set<Comp>>(d_components);
        return comp_set.insert(apx::to_index(entity), component);
    }

    template <typename Comp>
    void remove(apx::entity entity)
    {
        static_assert(tuple_contains_v<Comp>);
        assert(valid(entity));
        
        auto& comp_set = std::get<apx::sparse_set<Comp>>(d_components);
        return remove(entity, comp_set);
    }

    template <typename Comp>
    bool has(apx::entity entity)
    {
        static_assert(tuple_contains_v<Comp>);
        assert(valid(entity));

        auto& comp_set = std::get<apx::sparse_set<Comp>>(d_components);
        return comp_set.has(apx::to_index(entity));
    }

    template <typename Comp>
    Comp& get(apx::entity entity)
    {
        static_assert(tuple_contains_v<Comp>);
        assert(valid(entity));
        assert(has<Comp>(entity));

        auto& comp_set = std::get<apx::sparse_set<Comp>>(d_components);
        return comp_set[apx::to_index(entity)];
    }
};

}