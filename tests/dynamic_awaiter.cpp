#include <gtest/gtest.h>

#include <coroutine>

#include "../dynamic_awaiter.h"

struct VoidSuspendAwaiter {
    bool await_ready() {
        return false;
    }

    void await_suspend(std::coroutine_handle<>) {
    }

    int await_resume() {
        return 42;
    }
};

struct BoolSuspendAwaiter {
    bool await_ready() {
        return true;
    }

    bool await_suspend(std::coroutine_handle<>) {
        return false;
    }

    int await_resume() {
        return 24;
    }
};

struct TestAwaiter {
    static int destruction_count;
    int id;

    TestAwaiter(int id) : id(id) {
    }

    ~TestAwaiter() {
        destruction_count++;
    }

    bool await_ready() {
        return true;
    }

    void await_suspend(std::coroutine_handle<>) {
    }

    int await_resume() {
        return id;
    }
};

int TestAwaiter::destruction_count = 0;

TEST(AwaiterTest, ConstructWithVoidSuspend) {
    auto awaiter = VoidSuspendAwaiter{};
    dynamic::Awaiter<int, 64> a(awaiter);
    auto handle = std::noop_coroutine();
    EXPECT_FALSE(a.await_ready());
    EXPECT_TRUE(a.await_suspend(handle));
    EXPECT_EQ(a.await_resume(), 42);
}

TEST(AwaiterTest, ConstructWithBoolSuspend) {
    dynamic::Awaiter<int, 64> a(BoolSuspendAwaiter{});
    auto handle = std::noop_coroutine();
    EXPECT_TRUE(a.await_ready());
    EXPECT_FALSE(a.await_suspend(handle));
    EXPECT_EQ(a.await_resume(), 24);
}

TEST(AwaiterTest, AssignmentFromVoidToBool) {
    dynamic::Awaiter<int, 64> a(VoidSuspendAwaiter{});
    a = BoolSuspendAwaiter{};
    auto handle = std::noop_coroutine();
    EXPECT_TRUE(a.await_ready());
    EXPECT_FALSE(a.await_suspend(handle));
    EXPECT_EQ(a.await_resume(), 24);
}

TEST(AwaiterTest, AssignmentFromBoolToVoid) {
    dynamic::Awaiter<int, 64> a(BoolSuspendAwaiter{});
    a = VoidSuspendAwaiter{};
    auto handle = std::noop_coroutine();
    EXPECT_FALSE(a.await_ready());
    EXPECT_TRUE(a.await_suspend(handle));
    EXPECT_EQ(a.await_resume(), 42);
}

TEST(AwaiterTest, AssignmentDestructsOldAwaiter) {
    TestAwaiter::destruction_count = 0;
    auto awaiter = TestAwaiter{1};
    dynamic::Awaiter<int, 64> a(std::move(awaiter));
    auto awaiter_2 = TestAwaiter{2};
    a = std::move(awaiter_2);
    EXPECT_EQ(TestAwaiter::destruction_count, 1);
}

TEST(AwaiterTest, DestructsAwaiterOnScopeExit) {
    TestAwaiter::destruction_count = 0;
    auto awaiter = TestAwaiter{1};
    { dynamic::Awaiter<int, 64> a(std::move(awaiter)); }
    EXPECT_EQ(TestAwaiter::destruction_count, 1);
}
