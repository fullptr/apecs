#ifndef APECS_HPP_
#define APECS_HPP_

#include <cassert>
#include <coroutine>
#include <cstdint>
#include <deque>
#include <functional>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace apx {
namespace meta {

template <typename T, typename Tuple>
struct tuple_contains;

template <typename T, typename... Ts>
struct tuple_contains<T, std::tuple<Ts...>>
{
    static constexpr bool value = (std::is_same_v<T, Ts> || ...);
};

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

}

template <typename T> class generator;
template <typename T> class generator_iterator;

template <typename T>
class generator_promise
{
public:
    using value_type = std::decay_t<T>;

private:
    value_type*        d_val;
    std::exception_ptr d_exception;

public:
    generator_promise() = default;

    generator<T> get_return_object() noexcept
    {
        return generator<T>{
            std::coroutine_handle<generator_promise<T>>::from_promise(*this)
        };
    }

    constexpr std::suspend_always initial_suspend() const noexcept { return {}; }
    constexpr std::suspend_always final_suspend() const noexcept { return {}; }

    void return_void() {}

    std::suspend_always yield_value(value_type& value) noexcept
    {
        d_val = std::addressof(value);
        return {};
    }

    std::suspend_always yield_value(value_type&& value) noexcept
    {
        d_val = std::addressof(value);
        return {};
    }

    void unhandled_exception() { d_exception = std::current_exception(); }

    value_type& value() const noexcept { return *d_val; }

    void rethrow_if()
    {
        if (d_exception) {
            std::rethrow_exception(d_exception);
        }
    }
};

template <typename T>
class generator
{
public:
    using promise_type = generator_promise<T>;
    using value_type = typename promise_type::value_type;

    using iterator = generator_iterator<T>;

private:
    std::coroutine_handle<promise_type> d_coroutine;

    generator(const generator&) = delete;
    generator& operator=(generator) = delete;

public:
    explicit generator(std::coroutine_handle<promise_type> coroutine) noexcept
        : d_coroutine(coroutine)
    {}

    ~generator()
    {
        if (d_coroutine) { d_coroutine.destroy(); }
    }

    iterator begin()
    {
        advance();
        return iterator{*this};
    }

    iterator::sentinel end() noexcept
    {
        return {};
    }

    bool valid() const noexcept
    {
        return d_coroutine && !d_coroutine.done();
    }

    void advance()
    {
        if (d_coroutine) {
            d_coroutine.resume();
            if (d_coroutine.done()) {
                d_coroutine.promise().rethrow_if();
            }
        }
    }

    value_type& value() const
    {
        return d_coroutine.promise().value();
    }

    template <typename Func>
    void each(Func&& f)
    {
        for (auto& val : *this) {
            f(val);
        }
    }
};

template <typename T>
class generator_iterator
{
public:
    using value_type = typename generator<T>::value_type;

    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;

    struct sentinel {};

private:
    generator<T>& d_owner;

public:
    explicit generator_iterator(generator<T>& owner) noexcept
        : d_owner(owner)
    {}

    friend bool operator==(const generator_iterator& it, sentinel)
    {
        return !it.d_owner.valid();
    }

    generator_iterator& operator++()
    {
        d_owner.advance();
        return *this;
    }

    generator_iterator& operator++(int) { return operator++(); }

    value_type& operator*() const noexcept
    {
        return d_owner.value();
    }

    value_type* operator->() const noexcept
    {
        return std::addressof(operator*());
    }
};

template <typename value_type>
class sparse_set
{
public:
    using index_type = std::size_t;

    using packed_type = std::vector<std::pair<index_type, value_type>>;
    using sparse_type = std::vector<index_type>;

    using iterator = typename packed_type::iterator;

private:
    static_assert(std::is_integral<index_type>());

    static constexpr index_type EMPTY = std::numeric_limits<index_type>::max();

    packed_type d_packed;
    sparse_type d_sparse;

    // Grows the sparse set so that the given index becomes valid.
    constexpr void assure(index_type index)
    {
        assert(!has(index));
        if (d_sparse.size() <= index) {
            d_sparse.resize(index + 1, EMPTY);
        }
    }

public:
    sparse_set() = default;

    // Inserts the given value into the specified index. If a value already
    // exists at that index, it is overwritten.
    constexpr value_type& insert(index_type index, const value_type& value)
    {
        assure(index);
        d_sparse[index] = d_packed.size();
        return d_packed.emplace_back(index, value).second;
    }

    constexpr value_type& insert(index_type index, value_type&& value)
    {
        assure(index);
        d_sparse[index] = d_packed.size();
        return d_packed.emplace_back(index, std::move(value)).second;
    }

    template <typename... Args>
    constexpr value_type& emplace(index_type index, Args&&... args)
    {
        assure(index);
        d_sparse[index] = d_packed.size();
        return d_packed.emplace_back(std::piecewise_construct,
                                     std::forward_as_tuple(index),
                                     std::forward_as_tuple(std::forward<Args>(args)...)).second;
    }

    // Returns true if the specified index contains a value, and false otherwise.
    [[nodiscard]] bool has(index_type index) const
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
    void erase(index_type index)
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

    value_type& operator[](index_type index)
    {
        assert(has(index));
        return d_packed[d_sparse[index]].second;
    }

    const value_type& operator[](index_type index) const
    {
        assert(has(index));
        return d_packed[d_sparse[index]].second;
    }

    // Provides a generator that loops over the packed set, which is fast but
    // results in undefined behaviour when removing elements.
    apx::generator<std::pair<const index_type, value_type>&> fast()
    {
        for (auto pair : d_packed) {
            co_yield pair;
        }
    }

    // Provides a generator that loops over the sparse set, which is slow but
    // allows for deleting elements while looping.
    apx::generator<std::pair<const index_type, value_type>&> safe()
    {
        for (auto index : d_sparse) {
            if (index != EMPTY) {
                co_yield d_packed[index];
            }
        }
    }
};

enum class entity : std::uint64_t {};
using index_t = std::uint32_t;
using version_t = std::uint32_t;

static constexpr apx::entity null{std::numeric_limits<std::uint64_t>::max()};

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
class handle;

template <typename... Comps>
class registry
{
public:
    template <typename T>
    using callback_t = std::function<void(apx::entity, const T&)>;

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
    void remove(apx::entity entity, apx::sparse_set<Comp>& component_set)
    {
        if (has<Comp>(entity)) {
            for (auto& cb : std::get<std::vector<callback_t<Comp>>>(d_on_remove)) {
                cb(entity, get<Comp>(entity));
            }
            component_set.erase(apx::to_index(entity));
        }
    }

    template <typename Comp>
    apx::sparse_set<Comp>& get_component_set()
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

    apx::entity create()
    {
        index_t index = (index_t)d_entities.size();
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

    [[nodiscard]] bool valid(apx::entity entity) noexcept
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

    [[nodiscard]] std::size_t size() const
    {
        return d_entities.size();
    }

    void clear()
    {
        for (auto [idx, entity] : d_entities.safe()) {
            destroy(entity);
        }
        d_entities.clear();
        d_pool.clear();
    }

    template <typename Comp>
    Comp& add(apx::entity entity, Comp&& component)
    {
        static_assert(apx::meta::tuple_contains<apx::sparse_set<Comp>, tuple_type>::value);
        assert(valid(entity));

        auto& comp_set = get_component_set<Comp>();
        auto& ret = comp_set.insert(apx::to_index(entity), std::forward<Comp>(component));
        for (auto cb : std::get<std::vector<callback_t<Comp>>>(d_on_add)) {
            cb(entity, ret);
        }
        return ret;
    }

    template <typename Comp, typename... Args>
    Comp& emplace(apx::entity entity, Args&&... args)
    {
        static_assert(apx::meta::tuple_contains<apx::sparse_set<Comp>, tuple_type>::value);
        assert(valid(entity));

        auto& comp_set = get_component_set<Comp>();
        auto& ret = comp_set.emplace(apx::to_index(entity), std::forward<Args>(args)...);
        for (auto cb : std::get<std::vector<callback_t<Comp>>>(d_on_add)) {
            cb(entity, ret);
        }
        return ret;
    }

    template <typename Comp>
    void remove(apx::entity entity)
    {
        static_assert(apx::meta::tuple_contains<apx::sparse_set<Comp>, tuple_type>::value);
        assert(valid(entity));
        
        auto& comp_set = get_component_set<Comp>();
        return remove(entity, comp_set);
    }

    template <typename Comp>
    bool has(apx::entity entity)
    {
        static_assert(apx::meta::tuple_contains<apx::sparse_set<Comp>, tuple_type>::value);
        assert(valid(entity));

        auto& comp_set = get_component_set<Comp>();
        return comp_set.has(apx::to_index(entity));
    }

    template <typename Comp>
    Comp& get(apx::entity entity)
    {
        static_assert(apx::meta::tuple_contains<apx::sparse_set<Comp>, tuple_type>::value);
        assert(has<Comp>(entity));

        auto& comp_set = get_component_set<Comp>();
        return comp_set[apx::to_index(entity)];
    }

    template <typename Comp>
    const Comp& get(apx::entity entity) const
    {
        static_assert(apx::meta::tuple_contains<apx::sparse_set<Comp>, tuple_type>::value);
        assert(has<Comp>(entity));

        auto& comp_set = get_component_set<Comp>();
        return comp_set[apx::to_index(entity)];
    }

    template <typename Comp>
    Comp* get_if(apx::entity entity) noexcept
    {
        static_assert(apx::meta::tuple_contains<apx::sparse_set<Comp>, tuple_type>::value);
        return has<Comp>(entity) ? &get<Comp>(entity) : nullptr;
    }

    apx::generator<apx::entity> all()
    {
        for (auto [index, entity] : d_entities.fast()) {
            co_yield entity;
        }
    }

    template <typename T, typename... Ts>
    apx::generator<apx::entity> view()
    {
        for (auto [index, component] : get_component_set<T>().fast()) {
            apx::entity& entity = d_entities[index];
            if ((has<Ts>(entity) && ...)) {
                co_yield entity;
            }
        }
    }
};

template <typename... Comps>
class handle
{
    apx::registry<Comps...>* d_registry;
    apx::entity              d_entity;

public:
    handle(apx::registry<Comps...>& r, apx::entity e) : d_registry(&r), d_entity(e) {}

    bool valid() noexcept { return d_registry->valid(d_entity); }
    void destroy() { d_registry->destroy(d_entity); }

    template <typename Comp>
    Comp& add(const Comp& component) { return d_registry->template add<Comp>(d_entity, component); }

    template <typename Comp>
    Comp& add(Comp&& component) { return d_registry->template add<Comp>(d_entity, std::move(component)); }

    template <typename Comp, typename... Args>
    Comp& emplace(Args&&... args) { return d_registry->template emplace<Comp>(d_entity, std::forward<Args>(args)...); }

    template <typename Comp>
    void remove() { d_registry->template remove<Comp>(d_entity); }

    template <typename Comp>
    bool has() { return d_registry->template has<Comp>(d_entity); }

    template <typename Comp>
    Comp& get() { return d_registry->template get<Comp>(d_entity); }

    template <typename Comp>
    const Comp& get() const { return d_registry->template get<Comp>(d_entity); }

    template <typename Comp>
    Comp* get_if() noexcept { return d_registry->template get_if<Comp>(d_entity); }
};

template <typename... Comps>
inline apx::handle<Comps...> create_from(apx::registry<Comps...>& registry)
{
    return {registry, registry.create()};
}

}

#endif // APECS_HPP_