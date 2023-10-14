use async_trait::async_trait;
use log;
use std::borrow::BorrowMut;
use std::cell::RefCell;
use std::collections::BTreeMap;
use std::fs::File;
use std::io::Read;
use std::sync::Arc;
use tokio::task::yield_now;
use tokio::{spawn, task, time};
use tokio::sync::Mutex;

use crate::nodes::*;
use edgelink_abstractions::nodes::*;
use edgelink_abstractions::red::FlowConfig;
use edgelink_abstractions::Variant;
use edgelink_abstractions::{engine::*, EdgeLinkError, Error, Result};

struct FlowState {
    nodes: Vec<Box<dyn FlowNodeBehavior>>,
    context: Variant,
}

struct FlowShared {
    state: Mutex<FlowState>,
}

pub struct Flow {
    config: FlowConfig,
    shared: Arc<FlowShared>,
}

impl Flow {
    pub fn new(
        flow_elem: &serde_json::Value,
        elements: &Vec<serde_json::Value>,
    ) -> anyhow::Result<Self> {
        println!("Fucking......");
        let flow_config: FlowConfig = serde_json::from_value(flow_elem.clone())?;
        println!("Fucked......");

        println!(
            "-- Loading flow (id={0}, label='{1}'):",
            flow_config.id, flow_config.label
        );

        for bnd in inventory::iter::<BuiltinNodeDescriptor> {
            println!(
                "-- kind={}, type-name={}",
                bnd.meta.kind, bnd.meta.type_name
            );
        }

        // let nodes = Vec::new();
        // nodes.push(Box::new())

        for bnd in inventory::iter::<BuiltinNodeDescriptor> {
            println!("-{}, --{}", bnd.meta.kind, bnd.meta.type_name);
        }

        Ok(Flow {
            config: flow_config,
            shared: Arc::new(FlowShared {
                state: Mutex::new(FlowState {
                    nodes: Vec::new(), // nodes,
                    context: Variant::Object(BTreeMap::new()),
                }),
            }),
        })
    }

    pub(crate) fn id(&self) -> u64 {
        self.config.id
    }

    pub(crate) async fn start(&mut self) {
        let mut state = self.shared.state.lock().await;
        println!("Starting Flow (id={0})...", self.config.id);
        for node in state.nodes.iter_mut() {
            node.start().await;
        }
    }

    pub(crate) async fn stop(&mut self) {
        let mut state = self.shared.state.lock().await;
        println!("Stopping Flow (id={0})...", self.config.id);
        for node in state.nodes.iter_mut() {
            node.stop().await;
        }
    }
}

#[async_trait]
impl FlowBehavior for Flow {
    fn id(&self) -> u64 {
        self.config.id
    }

    fn label(&self) -> &str {
        &self.config.label
    }

    fn config(&self) -> &FlowConfig {
        &self.config
    }
}
