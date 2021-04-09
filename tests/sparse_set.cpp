#include "apecs.hpp"
#include <gtest/gtest.h>

TEST(sparse_set, set_and_get)
{
    apx::sparse_set<int> set;

    set.insert(2, 5);
    ASSERT_TRUE(set.has(2));
    ASSERT_EQ(set[2], 5);
}

TEST(sparse_set, erase)
{
    apx::sparse_set<int> set;

    set.insert(2, 5);
    ASSERT_TRUE(set.has(2));

    set.erase(2);
    ASSERT_FALSE(set.has(2));
}

TEST(sparse_set, fast_with_one_element)
{
    apx::sparse_set<int> set;
    set[2] = 5;

    for (auto [key, value] : set.fast()) {
        ASSERT_EQ(key, 2);
        ASSERT_EQ(value, 5);
    }
}