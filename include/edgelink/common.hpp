#pragma once

namespace edgelink {

struct EDGELINK_EXPORT IClosable {
    virtual void close() noexcept = 0;
};

template <typename T> using Awaitable = boost::asio::awaitable<T>;

template <typename TFuncType> using Signal = boost::signals2::signal<TFuncType>;

}; // namespace edgelink
