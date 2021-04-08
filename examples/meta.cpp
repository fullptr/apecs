#include "examples/meta.hpp"
#include "apecs.hpp"

#include <tuple>
#include <iostream>
#include <type_traits>

namespace examples {

void meta()
{
    std::cout << "apx::meta::tuple_contains:\n";
    std::cout << std::boolalpha;

    bool value1 = apx::meta::tuple_contains_v<int, std::tuple<float, int, double>>;
    std::cout << "  tuple_contains_v<int, std::tuple<float, int, double>>: " << value1 << "\n";
    assert(value1 == true);

    bool value2 = apx::meta::tuple_contains_v<int, std::tuple<float, double>>;
    std::cout << "  tuple_contains_v<int, std::tuple<float, double>>:      " << value2 << "\n";
    assert(value2 == false);

    bool value3 = apx::meta::tuple_contains_v<int, std::tuple<int, std::string>>;
    std::cout << "  tuple_contains_v<int, std::tuple<int, std::string>>:   " << value3 << "\n";
    assert(value3 == true);

    std::cout << "apx::for_each:\n";
    std::tuple<int, float, double> t{2, 4.5f, 3.3};
    std::size_t count = 0;
    apx::meta::for_each(t, [&](auto&& elem) {
        std::cout << "  doubling " << elem << " to " << 2*elem << "\n";
        ++count;
    });
    std::cout << "  ran the lambda " << count << " times\n";
    assert(count == std::tuple_size<decltype(t)>());
}

}