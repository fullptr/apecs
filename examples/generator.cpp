#include "examples/generator.hpp"
#include "apecs.hpp"

#include <iostream>

namespace examples {
namespace {

apx::generator<int> get_ints()
{
    co_yield 1;
    co_yield 2;
    co_yield 4;
    co_yield 3;
}

}

void generator()
{
    std::cout << "printing values from get_ints()\n";
    for (int x : get_ints()) {
        std::cout << x << "\n";
    }
}

}