use std::net::{IpAddr, SocketAddr};
use std::sync::Arc;

use base64::prelude::*;
use serde::Deserialize;

use crate::define_builtin_flow_node;
use crate::runtime::flow::Flow;
use crate::runtime::nodes::*;

#[derive(Debug)]
enum UdpMulticast {
    No,
    Board,
    Multi,
}

impl<'de> Deserialize<'de> for UdpMulticast {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        match s.as_str() {
            "false" => Ok(UdpMulticast::No),
            "board" => Ok(UdpMulticast::Board),
            "multi" => Ok(UdpMulticast::Multi),
            _ => Err(serde::de::Error::invalid_value(
                serde::de::Unexpected::Str(&s),
                &"expected 'false' or 'board' or 'multi'",
            )),
        }
    }
}

#[derive(Debug, Clone, Copy)]
enum UdpIpV {
    V4,
    V6,
}

impl<'de> Deserialize<'de> for UdpIpV {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        match s.as_str() {
            "udp4" => Ok(UdpIpV::V4),
            "udp6" => Ok(UdpIpV::V6),
            _ => Err(serde::de::Error::invalid_value(
                serde::de::Unexpected::Str(&s),
                &"expected 'udp4' or 'udp6'",
            )),
        }
    }
}

struct UdpOutNode {
    state: FlowNodeState,
    config: UdpOutNodeConfig,
}

impl UdpOutNode {
    fn create(
        _flow: Arc<Flow>,
        state: FlowNodeState,
        _config: &RedFlowNodeConfig,
    ) -> crate::Result<Arc<dyn FlowNodeBehavior>> {
        let udp_config: UdpOutNodeConfig =
            serde_json::from_value(serde_json::Value::Object(_config.json.clone()))?;

        let node = UdpOutNode {
            state,
            config: udp_config,
        };
        Ok(Arc::new(node))
    }
}

#[derive(Deserialize, Debug)]
struct UdpOutNodeConfig {
    /// Remote address
    #[serde(deserialize_with = "crate::runtime::red::json::deser::string_to_ipaddr")]
    addr: Option<IpAddr>,

    /// Remote port
    #[serde(deserialize_with = "crate::runtime::red::json::deser::string_to_option_u16")]
    port: Option<u16>,

    /// Local address
    #[serde(deserialize_with = "crate::runtime::red::json::deser::string_to_ipaddr")]
    iface: Option<IpAddr>,

    /// Local port
    #[serde(deserialize_with = "crate::runtime::red::json::deser::string_to_option_u16")]
    outport: Option<u16>,

    ipv: UdpIpV,

    #[serde(default)]
    base64: bool,
    //multicast: UdpMulticast,
}

#[async_trait]
impl FlowNodeBehavior for UdpOutNode {
    fn state(&self) -> &FlowNodeState {
        &self.state
    }

    async fn run(self: Arc<Self>, stop_token: CancellationToken) {
        let local_addr: SocketAddr = match self.config.outport {
            Some(port) => SocketAddr::new(self.config.iface.unwrap(), port),
            _ => match self.config.ipv {
                UdpIpV::V4 => "0.0.0.0:0".parse().unwrap(),
                UdpIpV::V6 => "[::]:0".parse().unwrap(),
            },
        };

        match tokio::net::UdpSocket::bind(local_addr).await {
            Ok(socket) => {
                let socket = Arc::new(socket);
                while !stop_token.is_cancelled() {
                    let node = self.clone();
                    let cloned_socket = socket.clone();

                    with_uow(
                        node.clone().as_ref(),
                        stop_token.clone(),
                        |msg| async move {
                            let msg_guard = msg.read().await;
                            if let Some(payload) = msg_guard.get_property("payload") {
                                let remote_addr = std::net::SocketAddr::new(
                                    node.config.addr.unwrap(), // TODO FIXME
                                    node.config.port.unwrap(),
                                );

                                if let Some(bytes) = payload.as_bytes() {
                                    if node.config.base64 {
                                        let b64_str = BASE64_STANDARD.encode(bytes);
                                        let bytes = b64_str.as_bytes();
                                        cloned_socket.send_to(bytes, remote_addr).await.unwrap();
                                    } else {
                                        cloned_socket.send_to(bytes, remote_addr).await.unwrap();
                                    }
                                }
                                if let Some(bytes) = payload.to_bytes() {
                                    cloned_socket.send_to(&bytes, remote_addr).await.unwrap();
                                } else {
                                    log::warn!("Failed to convert payload into bytes");
                                }
                            }

                            Ok(())
                        },
                    )
                    .await;
                }
            }

            Err(e) => {
                log::error!("Can not bind local address: {:?}", e);
            }
        }
    }
}

define_builtin_flow_node!("udp out", UdpOutNode::create);