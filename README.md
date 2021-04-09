# apecs
A tiny entity component system. API is similar to Entt, but all component types must be declared
up front. This allows for the internals to be implemented without type erase, instead relying on
variadic templates; which learning more of was the reason for making this. Components are stored
in `apx::sparse_set` objects, which are essentially a pair of `std::vector`s, one sparse and one
packed, which allows for index lookup as well as tighly packed components. When deleting
components, the interals may suffle elements around to keep the array densely packed. This does
result in reordering, but really you shouldn't be relying on components being in any particular
order.

This is a single header file that can be placed in your own project. It has no dependencies
itself. The tests however require GTest to run.

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
