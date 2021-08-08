#include "apecs.hpp"
#include <gtest/gtest.h>

struct foo {
    int value = 0;
};
struct bar {};

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

TEST(registry, for_each_type)
{
    apx::registry<foo, bar> reg;
    apx::entity e = reg.create();
    std::size_t count = 0;

    apx::meta::for_each(reg.tags, [&](auto&& tag) {
        using T = decltype(tag.type());
        reg.on_add<T>([&](apx::entity entity, const T&) {
            ++count;
        });
    });

    apx::meta::for_each(reg.tags, [&](auto&& tag) {
        using T = decltype(tag.type());
        reg.add<T>(e, {});
    });

    ASSERT_TRUE(reg.has<foo>(e));
    ASSERT_TRUE(reg.has<bar>(e));
    ASSERT_EQ(count, 2);
}

TEST(registry, test_noexcept_get)
{
    apx::registry<foo, bar> reg;
    apx::entity e = reg.create();

    reg.add<foo>(e, {});

    foo* foo_get = reg.get_if<foo>(e);
    ASSERT_NE(foo_get, nullptr);

    bar* bar_get = reg.get_if<bar>(e);
    ASSERT_EQ(bar_get, nullptr);
}

TEST(registry_view, view_for_loop)
{
    apx::registry<foo, bar> reg;

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

TEST(registry_view, view_for_loop_multi)
{
    apx::registry<foo, bar> reg;

    auto e1 = reg.create();
    reg.emplace<foo>(e1);
    reg.emplace<bar>(e1);

    auto e2 = reg.create();
    reg.emplace<bar>(e2);

    auto e3 = reg.create();
    reg.emplace<foo>(e3);
    reg.emplace<bar>(e3);

    std::size_t count = 0;
    for (auto entity : reg.view<foo, bar>()) {
        ++count;
    }
    ASSERT_EQ(count, 2);
}

TEST(registry_all, all_for_loop)
{
    apx::registry<foo, bar> reg;

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

TEST(registry, test_add)
// registry::add was actually broken when trying to call with an lvalue reference, but no
// other tests at the time tested this; they either used emplace passed an rvalue to add
// which worked fine. This test makes sure add works as expected, and allow both explicit
// typing and type deduction.
{
    apx::registry<foo> reg;

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

TEST(registry, multi_destroy_vector)
{
    apx::registry<foo> reg;
    auto e1 = reg.create();
    auto e2 = reg.create();
    auto e3 = reg.create();
    ASSERT_EQ(reg.size(), 3);

    std::vector v{e1, e2, e3};
    reg.destroy(v);
}

TEST(registry, multi_destroy_initializer_list)
{
    apx::registry<foo> reg;
    auto e1 = reg.create();
    auto e2 = reg.create();
    auto e3 = reg.create();
    ASSERT_EQ(reg.size(), 3);

    reg.destroy({e1, e2, e3});
}