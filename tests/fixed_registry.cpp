#include "apecs.hpp"
#include <gtest/gtest.h>

struct foo {
    int value = 0;
};
struct bar {};

TEST(fixed_registry, entity_invalid_after_destroying)
{
    apx::fixed_registry<foo, bar> reg;

    auto e = reg.create();
    ASSERT_TRUE(reg.valid(e));

    reg.destroy(e);
    ASSERT_FALSE(reg.valid(e));
}

TEST(fixed_registry, size_of_fixed_registry)
{
    apx::fixed_registry<foo, bar> reg;

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

TEST(fixed_registry, on_add_callback)
{
    apx::fixed_registry<foo, bar> reg;
    std::size_t count = 0;
    reg.on_add<foo>([&](apx::entity, const foo& component) {
        ++count;
    });

    auto e1 = reg.create();
    reg.add<foo>(e1, {});
    reg.add<bar>(e1, {}); // Should not increase the count

    auto e2 = reg.create();
    reg.add<foo>(e2, {});

    ASSERT_EQ(count, 2);
}

TEST(fixed_registry, on_remove_callback)
{
    apx::fixed_registry<foo, bar> reg;
    std::size_t count = 0;
    reg.on_remove<foo>([&](apx::entity, const foo& component) {
        ++count;
    });

    auto e1 = reg.create();
    reg.add<foo>(e1, {});

    auto e2 = reg.create();
    reg.add<foo>(e2, {});

    reg.remove<foo>(e1);
    ASSERT_EQ(count, 1);
}

TEST(fixed_registry, on_remove_callback_fixed_registry_destructs)
{
    std::size_t count = 0;

    {
        apx::fixed_registry<foo, bar> reg;
        reg.on_remove<foo>([&](apx::entity, const foo& component) {
            ++count;
        });

        auto e1 = reg.create();
        reg.add<foo>(e1, {});

        auto e2 = reg.create();
        reg.add<foo>(e2, {});
    }

    ASSERT_EQ(count, 2);
}

TEST(fixed_registry, on_remove_callback_fixed_registry_cleared)
{
    std::size_t count = 0;

    apx::fixed_registry<foo, bar> reg;
    reg.on_remove<foo>([&](apx::entity, const foo& component) {
        ++count;
    });

    auto e1 = reg.create();
    reg.add<foo>(e1, {});

    auto e2 = reg.create();
    reg.add<foo>(e2, {});

    reg.clear();
    ASSERT_EQ(count, 2);
}

TEST(fixed_registry, for_each_type)
{
    apx::fixed_registry<foo, bar> reg;
    apx::entity e = reg.create();
    std::size_t count = 0;

    apx::for_each(reg.tags, [&](auto&& tag) {
        using T = decltype(tag.type());
        reg.on_add<T>([&](apx::entity entity, const T&) {
            ++count;
        });
    });

    apx::for_each(reg.tags, [&](auto&& tag) {
        using T = decltype(tag.type());
        reg.add<T>(e, {});
    });

    ASSERT_TRUE(reg.has<foo>(e));
    ASSERT_TRUE(reg.has<bar>(e));
    ASSERT_EQ(count, 2);
}

TEST(fixed_registry, test_noexcept_get)
{
    apx::fixed_registry<foo, bar> reg;
    apx::entity e = reg.create();

    reg.add<foo>(e, {});

    foo* foo_get = reg.get_if<foo>(e);
    ASSERT_NE(foo_get, nullptr);

    bar* bar_get = reg.get_if<bar>(e);
    ASSERT_EQ(bar_get, nullptr);
}

TEST(fixed_registry_view, view_for_loop)
{
    apx::fixed_registry<foo, bar> reg;

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

TEST(fixed_registry_view, view_callback)
{
    apx::fixed_registry<foo, bar> reg;

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

TEST(fixed_registry_view, view_extended_callback)
{
    apx::fixed_registry<foo, bar> reg;

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

TEST(fixed_registry_all, all_for_loop)
{
    apx::fixed_registry<foo, bar> reg;

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

TEST(fixed_registry_all, all_callback)
{
    apx::fixed_registry<foo, bar> reg;

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

TEST(fixed_registry, test_add)
// fixed_registry::add was actually broken when trying to call with an lvalue reference, but no
// other tests at the time tested this; they either used emplace passed an rvalue to add
// which worked fine. This test makes sure add works as expected, and allow both explicit
// typing and type deduction.
{
    apx::fixed_registry<foo> reg;

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