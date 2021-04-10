# apecs: A Petite Entity Component System
A header-only, very small entity component system with no external dependencies. Simply pop the
header into your own project and off you go!

The API is very similar to EnTT, with the main difference being that all component types must
be declared up front. This allows for an implementation that doesn't rely on type erasure, which
in turn allows for more compile-time optimisations.

Components are stored contiguously in `apx::sparse_set` objects, which are essentially a pair
of `std::vector`s, one sparse and one packed, which allows for fast iteration over components.
When deleting components, these sets may reorder themselves to maintain tight packing; as such,
sorting isn't currently possibly, but also shouldn't be desired.

This library also includes some very basic meta-programming functionality, found in the
`apx::meta` namespace, as well as `apx::generator<T>`, a generator built using the C++20
coroutine API.

This project was just a fun little project to allow me to learn more about ECSs and how to
implement one, as well as metaprogramming and C++20 features. If you are building your own
project and need an ECS, I would recommend you build your own or use EnTT instead.

## Example
```cpp
struct Transform
{
  glm::vec3 position;
  glm::quat orientation;
  glm::vec3 scale;  
};

struct Mesh
{
  std::vector<Vertex> vertices;
};

int main() {
  apx::registry<Transform, Mesh, Light, Sound> reg;

  auto e = reg.create(); // Create a new entity

  Transform t{{0, 0, 0}, {0, 0, 0, 1}, {2, 2, 2}};
  reg.add<Transform>(e, t); // Add components

  if (reg.has<Transform>(e)) { // Check if components exist
    auto& position = reg.get<Transform>(e).position; // Get components
    reg.remove<Transform>(e); // Remove components
  }

  for (auto entity : reg.view<Mesh>()) { // Loop over entities that have a particular component
    render(entity);
  }

  for (auto entity : reg.view<Transform, Light>()) { // Multi-component views
    light_scene(entity);
  }

  for (auto entity : reg.all()) { // Loop over everything
    log(entity);
  }

  reg.destroy(e); // Delete an entity and all of its components.
}
```
