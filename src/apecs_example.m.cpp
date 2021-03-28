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
    fmt::print("{}\n", apx::meta::tuple_contains<int, std::tuple<float, int, std::string>>::value);
    fmt::print("{}\n", apx::meta::tuple_contains<int, std::tuple<float, std::string>>::value);
    fmt::print("{}\n", apx::meta::tuple_contains<int, std::tuple<int, float, std::string>>::value);
    fmt::print("{}\n", apx::meta::tuple_contains<int, std::tuple<float, int, int, std::string>>::value);
    fmt::print("{}\n", apx::meta::tuple_contains<int, std::tuple<int, float, int, std::string>>::value);
    return 0;
}