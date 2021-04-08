#include "apecs.hpp"

#include <gtest/gtest.h>

namespace {

apx::generator<int> get_ints()
{
    co_yield 1;
    co_yield 2;
    co_yield 4;
    co_yield 3;
}

}