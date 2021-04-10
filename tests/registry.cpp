#include "apecs.hpp"
#include <gtest/gtest.h>

namespace {

struct foo {};
struct bar {};

}

TEST(registry, entity_invalid_after_destroying)
{
    apx::registry<foo, bar> reg;

    auto e = reg.create();
    ASSERT_TRUE(reg.valid(e));

    reg.destroy(e);
    ASSERT_FALSE(reg.valid(e));
}

TEST(registry, size_of_registry)
{
    apx::registry<foo, bar> reg;

    auto e1 = reg.create();
    ASSERT_EQ(reg.size(), 1);

    auto e2 = reg.create();
    ASSERT_EQ(reg.size(), 2);

    auto e3 = reg.create();
    ASSERT_EQ(reg.size(), 3);

    reg.destroy(e2);
    ASSERT_EQ(reg.size(), 2);

    reg.clear();
    ASSERT_EQ(reg.size(), 0);
}