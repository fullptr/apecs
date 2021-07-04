#include "apecs.hpp"
#include <gtest/gtest.h>

struct foo {
    int value = 0;
};
struct bar {};

TEST(registry, entity_invalid_after_destroying)
{
    apx::registry reg;

    auto e = reg.create();
    ASSERT_TRUE(reg.valid(e));

    reg.destroy(e);
    ASSERT_FALSE(reg.valid(e));
}

TEST(registry, size_of_registry)
{
    apx::registry reg;

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

TEST(registry, test_noexcept_get)
{
    apx::registry reg;
    apx::entity e = reg.create();

    reg.add<foo>(e, {});

    foo* foo_get = reg.get_if<foo>(e);
    ASSERT_NE(foo_get, nullptr);

    bar* bar_get = reg.get_if<bar>(e);
    ASSERT_EQ(bar_get, nullptr);
}

TEST(registry_view, view_for_loop)
{
    apx::registry reg;

    auto e1 = reg.create();
    reg.emplace<foo>(e1);
    reg.emplace<bar>(e1);

    auto e2 = reg.create();
    reg.emplace<bar>(e2);

    std::size_t count = 0;
    for (auto entity : reg.view<foo>()) {
        ++count;
    }
    ASSERT_EQ(count, 1);
}

TEST(registry_view, view_callback)
{
    apx::registry reg;

    auto e1 = reg.create();
    reg.emplace<foo>(e1);
    reg.emplace<bar>(e1);

    auto e2 = reg.create();
    reg.emplace<bar>(e2);

    std::size_t count = 0;
    reg.view<foo>([&](apx::entity) {
        ++count;
    });
    ASSERT_EQ(count, 1);
}

TEST(registry_view, view_extended_callback)
{
    apx::registry reg;

    auto e1 = reg.create();
    reg.emplace<foo>(e1);
    reg.emplace<bar>(e1);

    auto e2 = reg.create();
    reg.emplace<bar>(e2);

    std::size_t count = 0;
    reg.view<foo>([&](apx::entity, foo& comp) {
        ++count;
        comp.value = 10;
    });
    ASSERT_EQ(count, 1);
    ASSERT_EQ(reg.get<foo>(e1).value, 10);
}

TEST(registry_all, all_for_loop)
{
    apx::registry reg;

    auto e1 = reg.create();
    reg.emplace<foo>(e1);
    reg.emplace<bar>(e1);

    auto e2 = reg.create();
    reg.emplace<bar>(e2);

    std::size_t count = 0;
    for (auto entity : reg.all()) {
        ++count;
    }
    ASSERT_EQ(count, 2);
}

TEST(registry_all, all_callback)
{
    apx::registry reg;

    auto e1 = reg.create();
    reg.emplace<foo>(e1);
    reg.emplace<bar>(e1);

    auto e2 = reg.create();
    reg.emplace<bar>(e2);

    std::size_t count = 0;
    reg.all([&](apx::entity) {
        ++count;
    });
    ASSERT_EQ(count, 2);
}

TEST(registry, test_add)
{
    apx::registry reg;

    { // lvalue ref, explicit type
        apx::entity e = reg.create();
        foo f;
        reg.add<foo>(e, f);
        ASSERT_TRUE(reg.has<foo>(e));
    }

    { // rvalue ref, explicit type
        apx::entity e = reg.create();
        reg.add<foo>(e, {});
        ASSERT_TRUE(reg.has<foo>(e));
    }

    { // lvalue ref, type deduced
        apx::entity e = reg.create();
        foo f;
        reg.add(e, f);
        ASSERT_TRUE(reg.has<foo>(e));
    }

    { // rvalue ref, type deduced
        apx::entity e = reg.create();
        reg.add(e, foo{});
        ASSERT_TRUE(reg.has<foo>(e));
    }
}