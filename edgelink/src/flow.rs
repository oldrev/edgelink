use async_trait::async_trait;
use log;
use std::borrow::BorrowMut;
use std::cell::RefCell;
use std::collections::BTreeMap;
use std::fs::File;
use std::io::Read;
use std::sync::Arc;
use tokio::sync::Mutex;
use tokio::task::yield_now;
use tokio::{spawn, task, time};

use crate::nodes::*;
use edgelink_abstractions::nodes::*;
use edgelink_abstractions::red::FlowConfig;
use edgelink_abstractions::red::JsonValues;
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
    pub fn new(flow_elem: &serde_json::Value, json_values: &JsonValues) -> anyhow::Result<Self> {

        let flow = Flow {
            config: serde_json::from_value(flow_elem.clone())?,
            shared: Arc::new(FlowShared {
                state: Mutex::new(FlowState {
                    nodes: Vec::new(), // nodes,
                    context: Variant::Object(BTreeMap::new()),
                }),
            }),
        };

        Ok(flow)
    }

    pub(crate) fn id(&self) -> u64 {
        self.config.id
    }

    pub(crate) async fn start(&mut self) {
        let mut state = self.shared.state.lock().await;
        dbg!("Starting Flow (id={0})...", self.config.id);
        for node in state.nodes.iter_mut() {
            node.start().await;
        }
    }

    pub(crate) async fn stop(&mut self) {
        let mut state = self.shared.state.lock().await;
        dbg!("Stopping Flow (id={0})...", self.config.id);
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
