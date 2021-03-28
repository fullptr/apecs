#include "apecs.hpp"
#include <gtest/gtest.h>

namespace {

apx::generator<int> ints()
{
    co_yield 1;
    co_yield 2;
    co_yield 3;
}

}

TEST(generator, sum_elements_of_ints)
{
    std::size_t count = 0;
    for (int value : ints()) {
        count += value;
    }
    ASSERT_EQ(count, 6);
}