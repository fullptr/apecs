#include "apecs.hpp"
#include <gtest/gtest.h>

struct foo {};
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

TEST(registry, on_add_callback)
{
    apx::registry<foo, bar> reg;
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

TEST(registry, on_remove_callback)
{
    apx::registry<foo, bar> reg;
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

TEST(registry, on_remove_callback_reigstry_destructs)
{
    std::size_t count = 0;

    {
        apx::registry<foo, bar> reg;
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

TEST(registry, for_each_type)
{
    apx::registry<foo, bar> reg;
    auto e = reg.create();

    apx::meta::for_each(reg.tags, [&](auto&& tag) {
        using T = decltype(apx::meta::from_tag(tag));
        reg.add<T>(e, {});
    });

    ASSERT_TRUE(reg.has<foo>(e));
    ASSERT_TRUE(reg.has<bar>(e));
}