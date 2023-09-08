// https://stackoverflow.com/questions/70798478/multiple-coroutines-using-shared-resource-how-to-synchronize-them?noredirect=1&lq=1

#pragma once

namespace edgelink::async {

/// @brief 异步锁
/// @tparam TExecutor
template <typename TExecutor> class AsyncLock {
  public:
    explicit AsyncLock(TExecutor& executor) : _exe{executor} {}

    AsyncLock(const AsyncLock&) = delete;
    AsyncLock(AsyncLock&&) = delete;
    AsyncLock& operator=(const AsyncLock&) = delete;
    AsyncLock& operator=(AsyncLock&&) = delete;

    Awaitable<void> async_lock() {
        // spdlog::debug("AsyncLock: async_lock");

        if (_counter++ == 0) {
            spdlog::debug("AsyncLock: async_lock first lock, retrun");
            co_return;
        }

        // spdlog::debug("AsyncLock: async_lock, aquire");

        auto ec = boost::system::error_code{};
        auto timer = boost::asio::steady_timer{_exe};
        timer.expires_after(boost::asio::steady_timer::duration::max());
        _waiters.push(&timer);

        // spdlog::debug("AsyncLock: waiting...");
        co_await timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        // spdlog::debug("AsyncLock: done...");
    }

    void unlock() {
        if (_counter.load() == 0) {
            return;
        }

        --_counter;

        if (_waiters.empty()) {
            return;
        }

        // spdlog::debug("AsyncLock: unlock");

        auto* next = _waiters.front();
        next->cancel();

        _waiters.pop();
    }

  private:
    TExecutor& _exe;
    std::queue<boost::asio::steady_timer*> _waiters{};
    std::atomic<int> _counter;
};

}; // namespace edgelink::async