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
        return false;
    }

    bool await_suspend(std::coroutine_handle<>) {
        return true;
    }

    int await_resume() {
        return 42;
    }
};

struct CounterAwaiter {
    static int destructionCount;
    static int copyCount;
    static int moveCount;

    static void resetCounts() {
        destructionCount = 0;
        copyCount = 0;
        moveCount = 0;
    }

    CounterAwaiter() = default;

    CounterAwaiter(const CounterAwaiter&) {
        copyCount++;
    }

    CounterAwaiter(CounterAwaiter&&) {
        moveCount++;
    }

    ~CounterAwaiter() {
        destructionCount++;
    }

    bool await_ready() {
        return false;
    }

    void await_suspend(std::coroutine_handle<>) {
    }

    int await_resume() {
        return 42;
    }
};

int CounterAwaiter::destructionCount = 0;
int CounterAwaiter::copyCount = 0;
int CounterAwaiter::moveCount = 0;

void checkAwaiter(dynamic::Awaiter<int, 1>& a) {
    auto handle = std::noop_coroutine();
    EXPECT_FALSE(a.await_ready());
    EXPECT_TRUE(a.await_suspend(handle));
    EXPECT_EQ(a.await_resume(), 42);
}

TEST(Constructor, ConstructWithVoidSuspend) {
    dynamic::Awaiter<int, 1> a(VoidSuspendAwaiter{});
    checkAwaiter(a);
}

TEST(Constructor, ConstructWithBoolSuspend) {
    dynamic::Awaiter<int, 1> a(BoolSuspendAwaiter{});
    checkAwaiter(a);
}

TEST(Assignment, AssignmentFromVoidToBool) {
    dynamic::Awaiter<int, 1> a(VoidSuspendAwaiter{});
    checkAwaiter(a);
    a = BoolSuspendAwaiter{};
    checkAwaiter(a);
}

TEST(Assignment, AssignmentFromBoolToVoid) {
    dynamic::Awaiter<int, 1> a(BoolSuspendAwaiter{});
    checkAwaiter(a);
    a = VoidSuspendAwaiter{};
    checkAwaiter(a);
}

TEST(Destructor, AssignmentDestructsOldAwaiter) {
    CounterAwaiter::resetCounts();
    auto awaiter_1 = CounterAwaiter();
    dynamic::Awaiter<int, 1> a(awaiter_1);
    auto awaiter_2 = CounterAwaiter();
    a = awaiter_2;
    EXPECT_EQ(CounterAwaiter::destructionCount, 1);
    checkAwaiter(a);
}

TEST(Destructor, DestructsAwaiterOnScopeExit) {
    CounterAwaiter::resetCounts();
    auto awaiter = CounterAwaiter();
    {
        dynamic::Awaiter<int, 1> a(awaiter);
        checkAwaiter(a);
    }
    EXPECT_EQ(CounterAwaiter::destructionCount, 1);
}

TEST(IsCopyable, ConstructAndAssign) {
    CounterAwaiter::resetCounts();
    auto awaiter = CounterAwaiter();
    dynamic::Awaiter<int, 1> a(awaiter);
    checkAwaiter(a);
    dynamic::Awaiter<int, 1> b(a);
    a = b;
    checkAwaiter(a);
    checkAwaiter(b);
    EXPECT_EQ(CounterAwaiter::copyCount, 3);
    EXPECT_EQ(CounterAwaiter::moveCount, 0);
}

TEST(IsMoveable, ConstructAndAssign) {
    CounterAwaiter::resetCounts();
    auto awaiter = CounterAwaiter();
    dynamic::Awaiter<int, 1> a(std::move(awaiter));
    checkAwaiter(a);
    dynamic::Awaiter<int, 1> b(std::move(a));
    checkAwaiter(b);
    a = std::move(b);
    checkAwaiter(a);
    EXPECT_EQ(CounterAwaiter::copyCount, 0);
    EXPECT_EQ(CounterAwaiter::moveCount, 3);
}
