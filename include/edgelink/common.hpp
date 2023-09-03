#pragma once

namespace edgelink {

struct IClosable {
    virtual void close() noexcept = 0;
};

template <typename T> using Awaitable = boost::asio::awaitable<T>;

}; // namespace edgelink
