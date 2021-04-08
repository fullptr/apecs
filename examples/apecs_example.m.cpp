#include "apecs.hpp"
#include "examples/meta.hpp"
#include "examples/sparse_set.hpp"

#include <iostream>

int main()
{
    std::cout << "Apecs examples\n\n";

    std::cout << "Example 1: Meta library\n";
    examples::meta();
    std::cout << "\n";

    std::cout << "Example 2: Sparse set\n";
    examples::sparse_set();
    std::cout << "\n";

    return 0;
}