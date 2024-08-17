# EdgeLink: A Node-RED Compatible Runtime Engine in Rust

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
git clone https://github.com/edge-link/edgelink.rs.git
cd edgelink.rs
```

### 2. Build

```bash
cargo build --release
```

### 3. Run

```bash
./target/release/edgelinkd
```

Or:

```bash
cargo run
```

## Configuration

Adjust various settings in the configuration file, such as port number, database connection, etc. Refer to [CONFIG.md](docs/CONFIG.md) for more information.

## Contribution

Contributions are welcome! Please read [CONTRIBUTING.md](.github/CONTRIBUTING.md) for more details.

## Project Status

**Alpha Development Stage**: The project is currently in the alpha development stage and cannot guarantee stable operation.

### The Current Status of Nodes:

- Core nodes:
    - Common nodes:
        - [x] Junction
        - [x] Inject
        - [-] Debug
        - [-] Complete
        - [ ] Catch
        - [ ] Status
        - [ ] Link
        - [ ] Comment
        - [ ] GlobalConfig
        - [ ] Unknown
    - Function nodes:
        - [ ] Function
        - [ ] Switch
        - [ ] Change
        - [ ] Range (WIP)
        - [ ] Temlate
        - [ ] Delay
        - [ ] Exec
        - [ ] Rbe/Filter (WIP)
    - Network nodes:
        - [ ] TLS
        - [ ] HTTP Proxy
        - [ ] MQTT
        - [ ] HTTP In
        - [ ] HTTP Request
        - [ ] WebSocket
        - [ ] TCP In
        - [ ] UDP
    - Parse nodes:
        - [ ] CSV
        - [ ] HTML
        - [ ] JSON
        - [ ] XML
        - [ ] YAML
    - Sqeuence nodes:
        - [ ] Split
        - [ ] Sort
        - [ ] Batch
    - Storage
        - [ ] File
        - [ ] Watch

## Roadmap

Check out our [roadmap](ROADMAP.md) to get a glimpse of the upcoming features and milestones.

## Known Issues

Please refer to [ISSUES.md](docs/ISSUES.md) for a list of known issues and workarounds.

## Feedback and Support

We welcome your feedback! If you encounter any issues or have suggestions, please open an [issue](https://github.com/edge-link/edgelink.rs/issues).

## License

This project is licensed under the Apache 2.0 License - see the [LICENSE](LICENSE) file for more details.
```