#ifndef GENERATOR_HPP
#define GENERATOR_HPP

// yields can be either a reference, or a movable object
template<typename Yields, typename Returns = void>
requires
    (std::is_reference_v<Yields> or std::is_move_constructible_v<Yields>) and
    (std::is_void_v<Returns> or std::is_reference_v<Returns> or std::is_move_constructible_v<Returns>)
class generator {
public:
    generator() noexcept = default;

    friend class promise_type;

    class promise_type {
        friend class generator<Yields, Returns>;

    public:
        auto get_return_object() noexcept {
            return generator<Yields, Returns>{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        constexpr auto initial_suspend() const noexcept { return std::suspend_always {}; }
        constexpr auto final_suspend() const noexcept { return std::suspend_always {}; }

        auto yield_value(Yields value) noexcept(std::is_reference_v<Yields> or std::is_nothrow_move_constructible_v<Yields>) {
            if constexpr (std::is_reference_v<Yields>)
                m_yielded = &value;
            else
                m_yielded.emplace(std::move(value));
            return std::suspend_always {};
        }

        template<typename T = Returns>
        requires std::is_void_v<T>
        auto return_void() noexcept {
            m_returned = this; // not null, hopefully...
            return std::suspend_always {};
        }

        template<typename T = Returns>
        requires (!std::is_void_v<T>)
        auto return_value(T value) noexcept(std::is_reference_v<Returns> or std::is_nothrow_move_constructible_v<Returns>) {
            if constexpr (std::is_reference_v<Returns>)
                m_returned = &value;
            else
                m_returned.emplace(std::move(value));
            return std::suspend_always {};
        }

        void unhandled_exception() {
            m_exception = std::current_exception();
        };

        template<typename T>
        auto await_transform(T&&) = delete;


        template<typename T>
        // TODO
        auto await_transform(generator<Yields, T> &other) noexcept {} 

        template<typename T>
        auto await_transform(generator<Yields, T> &&other) noexcept {
            return await_transform(other);
        }

        class iterator {
        public:
            explicit iterator(generator &gen) noexcept
                : m_generator{ &gen }
            {
                ++(*this);
            }

            iterator(iterator &&) noexcept = default;
            iterator(iterator const&) noexcept = default;
            iterator &operator=(iterator &&) noexcept = default;
            iterator &operator=(iterator const&) noexcept = default;

            bool operator==(std::default_sentinel_t) const noexcept {
                return m_generator->is_done();
            }

            // idk if the rethrow should be derefencing the iterator or increasing it
            // im going for the increment operator for now
            Yields operator *() noexcept(std::is_reference_v<Yields> or std::is_nothrow_move_constructible_v<Yields>) {
                promise_type &promise = m_generator->m_coroutine.promise();
                if constexpr (std::is_reference_v<Yields>)
                    // static_cast required for rvalue references
                    return static_cast<Yields>(*promise.m_yielded);
                else {
                    Yields result = std::move(promise.m_yielded.value());
                    return result;
                }
            }

            iterator& operator++() {
                promise_type &promise = m_generator->m_coroutine.promise();
                if constexpr (std::is_reference_v<Yields>)
                    promise.m_yielded = nullptr;
                else
                    promise.m_yielded.reset();
                while (not (m_generator->is_done() or promise.m_yielded)) {
                    m_generator->m_coroutine.resume();
                }
                if (promise.m_exception)
                   std::rethrow_exception(promise.m_exception);
                return *this;
            }
        private:
            generator<Yields, Returns> *m_generator;
        };
    private:
        // we use std::optional like storage space for the object
        std::conditional_t<std::is_reference_v<Yields>, std::remove_reference_t<Yields>*, std::optional<Yields>> m_yielded {};
        std::conditional_t<std::is_reference_v<Returns> or std::is_void_v<Returns>, std::remove_reference_t<Returns>*, std::optional<Returns>> m_returned {};
        std::exception_ptr m_exception = nullptr;
    };


    using iterator = typename promise_type::iterator;

    explicit generator(std::coroutine_handle<promise_type> handle) noexcept
        : m_coroutine{ handle }
    {}
    ~generator() {
        m_coroutine.destroy();
    }
    generator(generator&&) noexcept = default;
    generator(generator const&) = delete;
    generator &operator=(generator&&) noexcept = default;
    generator &operator=(generator const&) = delete;

    bool is_done() const noexcept {
        promise_type &promise = m_coroutine.promise();
        return promise.m_returned or promise.m_exception;
    }

    auto begin() noexcept {
        return iterator{ *this };
    }

    constexpr std::default_sentinel_t end() const noexcept { return {}; }

private:
    std::coroutine_handle<promise_type> m_coroutine = nullptr;
};


// the idea is the following:

#endif
