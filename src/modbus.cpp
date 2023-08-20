#include <iostream>
#include <string>
#include <span>

#include <boost/url.hpp>

#include <edgelink/edgelink.hpp>
#include <edgelink/modbus.hpp>

namespace edgelink {

ModbusClient::ModbusClient(const std::string_view& url, int baud, char parity, int data_bits, int stop_bits)
    : _modbus(nullptr), baud(baud), parity(parity), dataBits(data_bits), stopBits(stop_bits) {
    auto uri = boost::urls::parse_uri(url).value();

    if(uri.scheme() == "tcp") {
        _transport = ModbusTransport::TCP;
        _device = uri.host_address();
    }
    else if(uri.scheme() == "rtu") {
        _transport = ModbusTransport::RTU;
        _device = uri.host_address();
    }
    else {
        throw std::exception();
    }
    
}

ModbusClient::~ModbusClient() { disconnect(); }

bool ModbusClient::connect() {
    _modbus = modbus_new_rtu(_device.c_str(), baud, parity, dataBits, stopBits);
    if (_modbus == nullptr) {
        std::cerr << "Failed to create modbus context" << std::endl;
        return false;
    }

    if (modbus_connect(_modbus) == -1) {
        std::cerr << "Modbus connection failed: " << modbus_strerror(errno) << std::endl;
        modbus_free(_modbus);
        _modbus = nullptr;
        return false;
    }

    return true;
}

bool ModbusClient::disconnect() {
    if (_modbus != nullptr) {
        modbus_close(_modbus);
        modbus_free(_modbus);
        _modbus = nullptr;
        return true;
    }
    return false;
}

bool ModbusClient::read_input_registers(int address, std::span<uint16_t> data) {
    if (modbus_read_input_registers(_modbus, address, data.size(), data.data()) == -1) {
        std::cerr << "Modbus read input registers failed: " << modbus_strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool ModbusClient::write_single_register(int address, uint16_t value) {
    if (modbus_write_register(_modbus, address, value) == -1) {
        std::cerr << "Modbus write single register failed: " << modbus_strerror(errno) << std::endl;
        return false;
    }
    return true;
}

}; // namespace edgelink