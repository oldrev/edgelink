#pragma once

namespace edgelink {

#include <modbus/modbus.h>

enum class ModbusTransport {
    RTU = 0,
    TCP = 1,
};

class ModbusClient : public virtual IClosable {
  public:
    ModbusClient(const std::string_view& url, int baud, char parity, int data_bits, int stop_bits);
    ~ModbusClient();

    Result<> connect() noexcept;
    void close() noexcept override;
    Result<> read_input_registers(int address, std::span<uint16_t> data) noexcept;
    Result<> write_single_register(int address, uint16_t value) noexcept;

  private:
    modbus_t* _modbus;
    std::string _device;
    int baud;
    char parity;
    int dataBits;
    int stopBits;
    ModbusTransport _transport;
};

}; // namespace edgelink