use crate::model::ElementID;
use crate::msg::Msg;
use async_trait::async_trait;
use log;
use serde_json::Value as JsonValue;
use std::borrow::BorrowMut;
use std::cell::RefCell;
use std::collections::{BTreeMap, HashMap};
use std::fs::File;
use std::io::Read;
use std::sync::{Arc, Mutex, Weak};
use tokio::sync::{futures, Mutex as TokMutex};
use tokio::task::yield_now;
use tokio::{spawn, task, time};

use crate::engine::FlowEngine;
use crate::nodes::*;
use crate::red::json::RedFlowConfig;
use crate::registry::Registry;
use crate::variant::Variant;
use crate::{EdgeLinkError, Result};

struct FlowState {
    nodes: HashMap<ElementID, Box<dyn FlowNodeBehavior>>,
    nodes_ordering: Vec<ElementID>,
    context: Variant,
    engine: Weak<FlowEngine>,
}

struct FlowShared {
    state: TokMutex<FlowState>,
}

pub struct Flow {
    pub id: ElementID,
    pub label: String,
    pub disabled: bool,

    shared: Arc<FlowShared>,
}

impl Flow {
    pub fn id(&self) -> ElementID {
        self.id
    }

    pub fn label(&self) -> &str {
        &self.label
    }

    pub fn disabled(&self) -> bool {
        self.disabled
    }

    pub async fn emit(msg: Arc<Msg>) -> crate::Result<()> {
        Ok(())
    }

    pub(crate) async fn new(
        engine: Arc<FlowEngine>,
        flow_config: &RedFlowConfig,
        reg: Arc<dyn Registry>,
    ) -> crate::Result<Arc<Self>> {
        let flow: Arc<Flow> = Arc::new(Flow {
            id: flow_config.id,
            label: flow_config.label.clone(),
            disabled: flow_config.disabled.unwrap_or(false),
            shared: Arc::new(FlowShared {
                state: TokMutex::new(FlowState {
                    engine: Arc::downgrade(&engine),
                    nodes: HashMap::with_capacity(flow_config.nodes.len()),
                    nodes_ordering: Vec::new(),
                    context: Variant::empty_object(),
                }),
            }),
        });

        {
            let mut state = flow.shared.state.lock().await;

            for node_config in flow_config.nodes.iter() {
                if let Some(meta_node) = reg.get(&node_config.type_name) {
                    println!("-- Found type: {}", meta_node.type_name);
                    // 创建流节点实例
                    let node = match meta_node.factory {
                        NodeFactory::Flow(factory) => factory(flow.clone(), node_config),
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
                    state.nodes_ordering.push(node.id());
                    state.nodes.insert(node_config.id, node);
                }
            }
        }

        Ok(flow)
    }

    pub(crate) async fn start(&self) -> crate::Result<()> {
        let state = self.shared.state.lock().await;
        println!("-- Starting Flow (id={0})...", self.id);
        // 启动是按照节点依赖顺序的逆序
        for node_id in state.nodes_ordering.iter().rev() {
            let node = &state.nodes[node_id];
            println!("---- Starting Node (id={0}')...", node.id());
            node.start().await?;
        }
        Ok(())
    }

    pub(crate) async fn stop(&self) -> crate::Result<()> {
        let state = self.shared.state.lock().await;
        println!("-- Stopping Flow (id={0})...", self.id);
        for node_id in state.nodes_ordering.iter() {
            let node = &state.nodes[node_id];
            node.stop().await?;
        }
        Ok(())
    }
}
