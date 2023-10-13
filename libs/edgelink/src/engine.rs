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
use topo_sort::{SortResults, TopoSort};

use crate::flow::Flow;
use crate::nodes::*;
use edgelink_abstractions::red::{FlowConfig, RedNodeJsonValue};
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

        let json_value: serde_json::Value = serde_json::from_str(&contents)?;

        let mut topo_sort = TopoSort::new();
        let mut flow_values = Vec::new();
        let mut node_values = Vec::new();
        if let Some(all_values) = json_value.as_array() {
            for e in all_values.iter() {
                if let Some(item_type) = e["type"].as_str() {
                    if item_type == "tab" {
                        flow_values.push(e.clone());
                    } else {
                        node_values.push(e);
                        let id = e["id"].as_str().unwrap();
                        let deps = e.get_flow_node_dependencies();
                        topo_sort.insert_from_set(id, deps);
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

            /* 
            match topo_sort.into_vec_nodes() {
                SortResults::Full(nodes) => assert_eq!(vec!["A", "B", "C", "E", "D"], nodes),
                SortResults::Partial(_) => panic!("unexpected cycle!"),
            }
            */

            // load flows
            let mut flows = Vec::new();
            for e in flow_values.iter() {
                let flow = Flow::new(&e, &all_values)?;
                flows.push(Box::new(flow));
            }
        } else {
            return Err(EdgeLinkError::BadFlowsJson("Bad flows.json".to_string()).into());
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
