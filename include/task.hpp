#ifndef TASK_HPP
#define TASK_HPP

#include <coroutine>
#include <exception>
#include <future> // std::future_errc

template<typename Returns>
class task;


template<>
class task<void> {
private:
    class co_awaiter;
    class when_ready_awaiter;
public:
    class promise_type {
        friend class co_awaiter;
        friend class when_ready_awaiter;
        friend class task<void>;
    public:   
        constexpr auto initial_suspend() const noexcept {
            return std::suspend_always{};
        }

        constexpr auto final_suspend() const noexcept {
            return std::suspend_always{};
        }

        auto get_return_object() noexcept {
            return task<void>{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        constexpr void return_void() noexcept {
            m_returned = true;
        }

        void unhandled_exception() noexcept {
            m_exception = std::current_exception();
        }

    private:
        std::exception_ptr m_exception = nullptr;
        bool m_returned = false;
    };

private:
    constexpr explicit task(std::coroutine_handle<promise_type> handle) noexcept
        : m_coroutine{ handle }
    {}

public:
    constexpr task() noexcept = default;
    constexpr task(task &&other) noexcept = default;
    constexpr task& operator=(task &&other) noexcept = default;
    task(task const &other) = delete;
    task& operator=(task const &other) = delete;
    ~task() {
        m_coroutine.destroy();
    }

private:
    struct co_awaiter {
        bool await_ready() const {
            return m_task->is_done();
        }
        void await_resume() const { 
            if (m_task->m_coroutine.promise().m_exception)
                std::rethrow_exception(m_task->m_coroutine.promise().m_exception);
        }
        auto await_suspend(std::coroutine_handle<>) const noexcept {
            return m_task->m_coroutine;
        }
        task<void> *m_task;
    };

public:
    co_awaiter operator co_await() & noexcept {
        return { this };
    }

    co_awaiter operator co_await() && noexcept {
        return { this };
    }

    bool is_done() const {
        if (!m_coroutine)
            throw std::make_error_code(std::future_errc::no_state);
        return m_coroutine.promise().m_exception or m_coroutine.promise().m_returned;
    }

private:
    struct when_ready_awaiter {
        bool await_ready() const {
            return m_task->is_done();
        }
        void await_resume() const noexcept {}
        auto await_suspend(std::coroutine_handle<>) const noexcept {
            return m_task->m_coroutine;
        }
        task<void> *m_task;
    };

public:
    when_ready_awaiter when_ready()  noexcept { // similar to operator co_await, but doesn't throw
        return { this };
    }

private:
    std::coroutine_handle<promise_type> m_coroutine = nullptr;
};

template<typename T>
class task<T &> {

};

template<typename T>
class task<T &&> {

};


#endif
