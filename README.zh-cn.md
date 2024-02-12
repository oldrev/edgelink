# EdgeLink：使用 Rust 开发的 Node-RED 兼容运行时

![Node-RED Rust Backend](assets/banner.jpg)

[English](README.md) | 简体中文

## 概述

EdgeLink 是一个使用 Rust 编写的 Node-RED 后端运行时，旨在提高性能并降低内存占用。通过将 Node-RED 的原 NodeJS 后端替换为这个基于 Rust 的实现，可以获得更好的性能和更小的内存足迹。

## 特性

- **高性能**: 使用 Rust 语言的优势，提供卓越的性能。
- **低内存占用**: 相比 NodeJS 后端，降低了内存使用。
- **可扩展性**: 保留了 Node-RED 的可扩展性，支持自定义节点。
- **轻松迁移**: 工作流尽可能兼容现有 Node-RED 工作流文件，可以直接利用 NodeRED 的设计器进行工作流开发和测试。

## 快速开始

### 1. 克隆仓库

```bash
git clone https://github.com/your-username/node-red-rust-backend.git
cd node-red-rust-backend
```

### 2. 构建

```bash
cargo build --release
```

### 3. 运行

```bash
./target/release/node-red-rust-backend
```

## 配置

在配置文件中可以调整各种设置，例如端口号、数据库连接等。请参考 [CONFIG.md](docs/CONFIG.md) 获取更多信息。

## 贡献

欢迎贡献！请阅读 [CONTRIBUTING.md](.github/CONTRIBUTING.md) 获取更多信息。

## 反馈与技术支持

我们欢迎任何反馈！如果你遇到任何技术问题或者 bug，请不要提交 [issue](https://github.com/edge-link/edgelink-rust/issues)。

## 许可证

此项目基于 Apache 2.0 许可证 - 详见 [LICENSE](LICENSE) 文件以获取更多详细信息。