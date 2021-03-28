#include "apecs.hpp"
#include <string_view>

struct transform
{
    float x, y, z;
};

struct model
{
    std::string_view model;
};

int main()
{
    apx::registry<int, float> reg;

    auto e1 = reg.create();
    auto e2 = reg.create();
    reg.add<int>(e1, 5);
    reg.add<int>(e2, 6);
    for (auto entity : reg.all()) {
        fmt::print("{}\n", entity);
    }
    return 0;
}