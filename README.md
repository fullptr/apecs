# apecs: A Petite Entity Component System
A header-only, very small entity component system with no external dependencies. Simply pop the header into your own project and off you go!

The API is very similar to EnTT, with the main difference being that all component types must be declared up front. This allows for an implementation that doesn't rely on type erasure, which in turn allows for more compile-time optimisations.

Components are stored contiguously in `apx::sparse_set` objects, which are essentially a pair of `std::vector`s, one sparse and one packed, which allows for fast iteration over components. When deleting components, these sets may reorder themselves to maintain tight packing; as such, sorting isn't currently possibly, but also shouldn't be desired.

This library also includes some very basic meta-programming functionality, found in the `apx::meta` namespace, as well as `apx::generator<T>`, a generator built using the C++20 coroutine API.

This project was just a fun little project to allow me to learn more about ECSs and how to implement one, as well as metaprogramming and C++20 features. If you are building your own project and need an ECS, I would recommend you build your own or use EnTT instead.

## The Registry and Entities
In `apecs`, an entity, `apx::entity`, is simply a 64-bit unsigned integer. All components attached to this entity are stored and accessed via the `apx::fixed_registry` class. To start, you can default construct a registry, with all of the component types declated up front
```cpp
apx::fixed_registry<transform, mesh, light, physics, script> registry;
```
There is also a newer version that is implemented using type erasure rather than variadix templates. Constructing one is cleaner, and the API is almost identical to the fixed registry. Constructing one is as simple as
```cpp
apx::registry registry;
```
Creating an empty entity is simple
```cpp
apx::entity e = registry.create();
```
Adding a component is also easy
```cpp
transform t = { 0.0, 0.0, 0.0 }; // In this example, a transform consists of just a 3D coordinate
registry.add(e, t);

// Or more explicitly:
registry.add<transform>(e, t);
```
Move construction is also allowed, as well as directly constructing via emplace
```cpp
// Uses move constructor (not that there is any benefit with the simple trasnsform struct)
registry.add<transform>(e, {0.0, 0.0, 0.0});

// Only constructs one instance and does no copying/moving
registry.emplace<transform>(e, 0.0, 0.0, 0.0);
```
Removing is just as easy
```cpp
registry.remove<transform>(e);
```
Components can be accessed by reference for modification, and entities may be queried to see if they contain the given component type
```cpp
if (registry.has<transform>(e)) {
  auto& t = registry.get<transform>(e);
  update_transform(t);
}
```
There is also the noexcept version `get_if` which returns a pointer to the component, and `nullptr` if it does not exist
```cpp
if (auto* t = registry.get_if<transform>(e)) {
  update_transform(*t);
}
```
Deleting an entity is also straightforward
```cpp
registry.destroy(e);
```
Given that an `apx::entity` is just an identifier for an entity, it could be that an identifier
is referring to an entity that has been destroyed. The registry provides a function to check this
```cpp
registry.valid(e); // Returns true if the entity is still active and false otherwise.
```
The current size of the registry is the number of currently active entities
```cpp
std::size_t active_entities = registry.size();
```
Finally, a registry may also be cleared of all entities with
```cpp
registry.clear();
```

## Iteration
Iteration is implmented using C++20 coroutines. There are two main ways of doing interation; iterating over all entities, and iterating over a *view*; a subset of the entities containing only a specific set of components.

Iterating over all
```cpp
for (auto entity : registry.all()) {
  ...
}
```
Iterating over a view
```cpp
for (auto entity : registry.view<transform, mesh>()) {
  ...
}
```
When iterating over all entities, the iteration is done over the internal entity sparse set. When iterating over a view, we iterate over the sparse set of the first specified component, which can result in a much faster loop. Because of this, if you know that one of the component types is rarer than the others, put that as the first component.

If you prefer a more functional approach, `all` and `view` may also accept lambdas:
```cpp
registry.all([](auto entity) {
  ...
});
```
and
```cpp
registry.view<transform, mesh>(auto entity) {
  ...
});
```
There is also an "extended" version of `view` to access the components more easily:
```cpp
registry.view<transform, mesh>(auto entity, const transform& t, const mesh& m) {
  ...
});
```

## Notification System
NOTE: This only applies to `apx::fixed_registry`. There is nothing stopping me from adding these to the typeless version, I just don't currently need it myself.

`apecs`, like `EnTT`, is mainly just a data structure for storing components, and does not have any built in features specifically for systems; they are left up to the user. However, `apecs` does also allow for registering callbacks so that systems can be notified whenever a component is created or destroyed. Callbacks have the signature `void(apx::entity, const Component&)`. To be notified of a component being added, use `on_add`
```cpp
registry.on_add<transform>([&](apx::entity entity, const transform& component) {
  ...
});
```
`on_add` callbacks are invoked **after** the component has been added. Similarly for removing, use `on_remove`
```cpp
registry.on_remove<transform>([&](apx::entity entity, const transform& component) {
  ...
});
```
`on_remove` callbacks are invoked **before** the component has been removed. It is currently not possible to remove callbacks, but this may be added in the future.

If a registry is cleared, all `on_remove` callbacks are invoked for each entity along the way.

## Entity Handle
To some, a call such as `registry.add<transform>(entity, t)` may feel unnatural and would prefer a more traditional object oriented interface such as `entity.add<transform>(t)`. This is provided via `apx::fixed_handle<Comps...>` and `apx::handle`, a thin wrapper around a registry pointer and an entity. Given a reigstry and an entity, a handle can be created easily
```cpp
apx::handle handle{registry, entity};
```
To make your code prettier, a helper function is provided to create handles when creating a fresh entity
```cpp
apx::handle entity = apx::create_from(registry);
```
As stated, this is a simple wrapper and provides the whole interface for a single entity
```cpp
handle.valid();
handle.destroy();

handle.add<transform>(t);
handle.emplace<transform>(args...);
handle.has<transform>();
handle.get<transform>();
handle.get_if<transform>();
handle.remove<transform>();
```

## Metaprogramming
To implement many of these features, some metaprogramming techniques were required and are made available to users. First of all, `apx::tuple_contains` allows for checking at compile time if a given `std::tuple` type contains a specific type. This is used in the component getter/setter functions to give nicer compile errors if there is a type problem, but may be useful in other situations.
```cpp
static_assert(apx::tuple_contains_v<int, std::tuple<float, int, std::string>> == true);
static_assert(apx::tuple_contains_v<int, std::tuple<float, std::string>> == false);
```
When destroying an entity, we also need to loop over all types to delete the components and to make sure any `on_remove` callbacks are invoked. This can be done with `apx::for_each`
```cpp
apx::meta::for_each(tuple, [](auto&& element) {
  ...
});
```
This of course needs to be generic lambda as this gets invoked for each typle in the tuple.

In extension to the above, you may also find yourself needing to loop over all types within a reigstry. This can be achieved by creating a tuple of `apx::tag<T>` types and extracting the type from those in a for loop. The library provides some helpers for this. In particular, each registry provides an `inline static constexpr` version of this tuple as `registry<Comps...>::tags` and there is `apx::meta::tag<T>::type()` which is intended to be used in a `declype` expression to get the type:
```cpp
apx::meta::for_each(registry.tags, [](auto&& tag) {
  using T = decltype(tag.type());
  ...
});
```
If your compiler supports explicit template types in lambdas (C++20 feature, not currently implemented by everyone), this can be simplified to
```cpp
apx::meta::for_each(registry.tags, []<typename T>(apx::meta::tag<T>) {
  ...
});
```

## Upcoming Features
- The ability to deregister callbacks.
- A slower way of iterating components that allows for deleting and inserting new components. See `apx::sparse_set::safe` vs `apx::sparse_set::fast` for more info.
- Potentially `apx::handle` based versions for `registry::all` and `registry::view`.