# EdgeLink: A Node-RED Compatible Runtime in Rust

![Node-RED Rust Backend](assets/banner.jpg)

English | [简体中文](README.zh-cn.md)

## Overview

This is a Node-RED compatible runtime implemented in Rust, designed to enhance performance and reduce memory footprint. By replacing the original NodeJS backend with this Rust-based implementation, you can achieve better performance and a smaller memory footprint.

## Features

- **High Performance**: Leverage the advantages of the Rust language for excellent performance.
- **Low Memory Footprint**: Reduce memory usage compared to the NodeJS backend.
- **Scalability**: Retain the extensibility of Node-RED, supporting custom nodes.
- **Easy Migration**: Easily replace the existing Node-RED backend with minimal modifications.

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/edge-link/edgelink-rust.git
cd edgelink-rust
```

### 2. Build

```bash
cargo build --release
```

### 3. Run

```bash
./target/release/edgelinkd
```

## Configuration

Adjust various settings in the configuration file, such as port number, database connection, etc. Refer to [CONFIG.md](docs/CONFIG.md) for more information.

## Contribution

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for more details.

## Project Status

**Alpha Development Stage**: The project is currently in the alpha development stage and cannot guarantee stable operation.

## Roadmap

Check out our [roadmap](ROADMAP.md) to get a glimpse of the upcoming features and milestones.

## Known Issues

Please refer to [ISSUES.md](docs/ISSUES.md) for a list of known issues and workarounds.

## Feedback and Support

We welcome your feedback! If you encounter any issues or have suggestions, please open an [issue](https://github.com/your-username/node-red-rust-backend/issues).

## License

This project is licensed under the Apache 2.0 License - see the [LICENSE](LICENSE) file for more details.
```