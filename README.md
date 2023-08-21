# EdgeLink - 物联网边缘网关数据采集与推送系统

![EdgeLink Logo](edge_link_logo.png)

EdgeLink 是一个用于物联网边缘网关的数据采集与推送系统，旨在帮助用户从多种接口（如Modbus TCP, Modbus RTU, OPC UA等）采集数据，并通过MQTT协议将数据推送到远程采集端。这个项目的目标是提供一个可靠、高效且易于配置的解决方案，以满足物联网应用的数据采集和传输需求。


## 特性

- **多接口支持**: EdgeLink 提供了对多种常见物联网接口的支持，包括 Modbus TCP, Modbus RTU, OPC UA 等，使您可以轻松地连接到不同类型的设备和传感器。

- **MQTT 数据推送**: EdgeLink 使用 MQTT 协议来实现数据的安全、可靠的推送。这种通信方式适用于远程监控、数据存储和分析等多种场景。

- **配置灵活**: EdgeLink 提供了灵活的配置选项，使用户能够根据其具体需求轻松定制数据采集和推送规则。

- **可扩展性**: 项目的架构允许轻松添加新的接口和协议支持，以适应不断变化的物联网生态系统。

- **安全性**: EdgeLink 强调数据的安全性，支持传输层加密和身份验证，确保数据在传输过程中得到保护。

## 快速开始

### 安装与配置

1. 安装库依赖
    ```bash
    sudo apt install libmodbus-dev libboost1.81-all-dev libboost-url1.81-dev
    ```

2. 克隆本仓库到您的设备上：

    ```bash
    git clone https://github.com/oldrev/edgelink-linux-device-side.git
    ```

3. 执行构建

    ```bash
    ./build.sh
    ```

根据您的需求，编辑配置文件 `edgelink-conf.json`，配置数据采集接口、MQTT服务器等参数。

### 运行 EdgeLink：

TODO

## 使用示例

以下是一个简单的使用示例，假设您的 EdgeLink 配置允许从 Modbus TCP 设备采集数据并将其发送到 MQTT 服务器。






