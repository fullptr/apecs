#include "examples/sparse_set.hpp"
#include "apecs.hpp"

#include <iostream>

namespace examples {

void sparse_set()
{
    std::cout << "fast loop over sparse_set\n";
    apx::sparse_set<int> set;
    set[3] = 2;
    set[7] = 1;
    set[9] = 0;
    set[1] = 1;
    for (auto [index, value] : set.fast()) {
        std::cout << index << " -> " << value << "\n";
    }
}
    
}