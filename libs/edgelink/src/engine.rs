use async_trait::async_trait;
use log;
use std::borrow::BorrowMut;
use std::cell::RefCell;
use std::collections::BTreeMap;
use std::fs::File;
use std::io::Read;
use std::sync::{Arc, Mutex};
use tokio::task::yield_now;
use tokio::{spawn, task, time};

use crate::flow::Flow;
use crate::nodes::*;
use crate::red::FlowConfig;
use edgelink_abstractions::Variant;
use edgelink_abstractions::{engine::*, EdgeLinkError, Error, Result};
use edgelink_abstractions::{nodes::*, Registry};

struct FlowEngineState {
    flows: Vec<Box<Flow>>,
    context: Variant,
}

struct FlowEngineShared {
    state: Mutex<FlowEngineState>,
}

pub struct FlowEngine {
    shared: Arc<FlowEngineShared>,
}

impl FlowEngine {
    pub fn new(reg: &dyn Registry, flows_json_path: &str) -> Result<Self> {
        let mut file = File::open(flows_json_path)?;
        let mut contents = String::new();
        file.read_to_string(&mut contents)?;

        let mut flows = Vec::new();
        let json_value: serde_json::Value = serde_json::from_str(&contents)?;
        if let Some(elements) = json_value.as_array() {
            for e in elements.iter() {
                if let Some(item_type) = e["type"].as_str() {
                    if item_type == "tab" {
                        // let flow = &Flow::new(&e, &elements)? as &dyn FlowBehavior;
                        let flow = Flow::new(&e, &elements)?;
                        flows.push(Box::new(flow));
                    } else {
                        if let Some(meta_node) = reg.get(item_type) {
                            println!("Available node: type={0}", meta_node.type_name);
                        } else {
                            return Err(EdgeLinkError::NotSupported(
                                "Bad Bad Bad Type".to_string(),
                            )
                            .into());
                        }
                    }
                }
            }
        }

        Ok(FlowEngine {
            shared: Arc::new(FlowEngineShared {
                state: Mutex::new(FlowEngineState {
                    flows: Vec::new(),
                    context: Variant::Object(BTreeMap::new()),
                }),
            }),
        })
    }

    pub async fn start(&mut self) {
        let mut state = self.shared.state.lock().unwrap();
        for flow in state.flows.iter_mut() {
            flow.start().await;
        }
    }

    pub async fn stop(&mut self) {
        let mut state = self.shared.state.lock().unwrap();
        for flow in state.flows.iter_mut() {
            flow.stop().await;
        }
    }
}

#[async_trait]
impl FlowEngineBehavior for FlowEngine {}
