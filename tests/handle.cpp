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

TEST(handle, test_add)
{
    apx::registry<foo> reg;

    { // lvalue ref, explicit type
        apx::handle h = apx::create_from(reg);
        foo f;
        h.add<foo>(f);
        ASSERT_TRUE(h.has<foo>());
    }

    { // rvalue ref, explicit type
        apx::handle h = apx::create_from(reg);
        h.add<foo>({});
        ASSERT_TRUE(h.has<foo>());
    }

    { // lvalue ref, type deduced
        apx::handle h = apx::create_from(reg);
        foo f;
        h.add(f);
        ASSERT_TRUE(h.has<foo>());
    }

    { // rvalue ref, type deduced
        apx::handle h = apx::create_from(reg);
        h.add(foo{});
        ASSERT_TRUE(h.has<foo>());
    }
}

TEST(handle, test_destroy_if)
// Test removing all but the first element.
{
    apx::registry<foo> reg;
    (void)reg.create();
    (void)reg.create();
    (void)reg.create();
    (void)reg.create();

    bool passed_first = false;
    reg.destroy_if([&](apx::entity e) {
        if (!passed_first) {
            passed_first = true;
            return false;
        }
        return true;
    });

    ASSERT_EQ(reg.size(), 1);
}