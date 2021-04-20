#include "apecs.hpp"
#include <gtest/gtest.h>

struct foo {};
struct bar {};

TEST(handle, handle_basics)
{
    apx::registry<foo, bar> reg;
    apx::handle h = apx::create_from(reg);

    h.emplace<foo>();
    ASSERT_TRUE(h.has<foo>());

    h.remove<foo>();
    ASSERT_FALSE(h.has<foo>());

    ASSERT_EQ(h.get_if<foo>(), nullptr);
}