#include "apecs.hpp"
#include <gtest/gtest.h>
#include <tuple>

TEST(meta, contains_true)
{
    static_assert(apx::meta::tuple_contains_v<int, std::tuple<float, int, double>>);
    static_assert(apx::meta::tuple_contains_v<int, std::tuple<int, double>>);
}

TEST(meta, contains_false)
{
    static_assert(!apx::meta::tuple_contains_v<int, std::tuple<float, double>>);
}

TEST(meta, tuple_for_each_calls_for_every_element)
{
    std::tuple<int, float, double> t;
    std::size_t count = 0;

    apx::meta::for_each(t, [&](auto&&) {
        ++count;
    });

    ASSERT_EQ(count, 3);
}