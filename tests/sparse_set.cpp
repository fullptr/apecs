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

TEST(sparse_set, iterate_with_one_element_and_index_check)
{
    apx::sparse_set<int> set;
    set.insert(2, 5);

    for (const auto& [index, value] : set.each()) {
        ASSERT_EQ(index, 2);
        ASSERT_EQ(value, 5);

    }
}