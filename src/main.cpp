
#include <task.hpp>
#include <generator.hpp>

task<void> test() {
    co_return;
}

task<void> test2() {
    co_await test(); // should use operator co_await() &&
    task<void> t = test();
    co_await t; // should use operator co_await() &
    co_return;
}

generator<std::uintmax_t> count(std::uintmax_t start = 0) {
    while (true) {
        co_yield start++;
    }
}

#include <iostream>

int main() {
    for (std::uintmax_t n : count()) {
        std::printf("%ju\n", n);
    }
}
