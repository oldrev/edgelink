use async_trait::async_trait;
use log;
use serde_json::Value as JsonValue;
use std::borrow::BorrowMut;
use std::cell::RefCell;
use std::collections::{BTreeMap, HashMap};
use std::fs::File;
use std::io::Read;
use std::sync::Arc;
use tokio::sync::{Mutex, futures};
use tokio::task::yield_now;
use tokio::{spawn, task, time};

use crate::nodes::*;
use edgelink_abstractions::red::JsonValues;
use edgelink_abstractions::red::{RedFlowConfig, RedFlowNodeConfig};
use edgelink_abstractions::Variant;
use edgelink_abstractions::{engine::*, EdgeLinkError, Error, Result};
use edgelink_abstractions::{nodes::*, Registry};

struct FlowState {
    nodes: Vec<Box<dyn FlowNodeBehavior>>,
    context: Variant,
}

struct FlowShared {
    state: Mutex<FlowState>,
}

pub struct Flow {
    id: u64,
    label: String,
    disabled: bool,

    shared: Arc<FlowShared>,
}

impl Flow {
    pub fn new(flow_config: &RedFlowConfig, reg: &dyn Registry) -> anyhow::Result<Self> {

        let mut nodes = Vec::with_capacity(flow_config.nodes.len());
        for node_config in flow_config.nodes.iter() {
            if let Some(meta_node) = reg.get(&node_config.type_name) {
                dbg!("Found type: {}", meta_node.type_name);
                // 创建流节点实例
                let node = match meta_node.factory {
                    NodeFactory::Flow(factory) => factory(node_config),
                    _ => {
                        return Err(EdgeLinkError::NotSupported(
                            format!(
                                "Can not found flow node factory for Node(id={0}, type='{1}'",
                                flow_config.id, flow_config.type_name
                            )
                            .to_string(),
                        )
                        .into())
                    }
                };
                nodes.push(node);
            }
        }

        let flow = Flow {
            id: flow_config.id,
            label: flow_config.label.clone(),
            disabled: flow_config.disabled.unwrap_or(false),
            shared: Arc::new(FlowShared {
                state: Mutex::new(FlowState {
                    nodes: Vec::new(),
                    context: Variant::Object(BTreeMap::new()),
                }),
            }),
        };

        Ok(flow)
    }

    pub(crate) async fn start(&self) -> Result<()> {
        let mut state = self.shared.state.lock().await;
        dbg!("Starting Flow (id={0})...", self.id);
        for node in state.nodes.iter_mut() {
            node.start().await?;
        }
        Ok(())
    }

    pub(crate) async fn stop(&self) -> Result<()> {
        let mut state = self.shared.state.lock().await;
        dbg!("Stopping Flow (id={0})...", self.id);
        for node in state.nodes.iter_mut() {
            node.stop().await?;
        }
        Ok(())
    }
}

#[async_trait]
impl FlowBehavior for Flow {
    fn id(&self) -> u64 {
        self.id
    }

    fn label(&self) -> &str {
        &self.label
    }

    fn disabled(&self) -> bool {
        self.disabled
    }
}
