#pragma once

class ModbusClient {
  public:
    ModbusClient() {}

    boost::asio::awaitable connect(const std::string_view& address) {
        auto executor = co_await boost::asio::this_coro::executor;
        serial_port port(io, address);
        co_await async_write(socket, boost::asio::buffer(data, n), use_awaitable);
    }

  private:
};