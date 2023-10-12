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

use crate::nodes::*;
use edgelink_abstractions::nodes::*;
use edgelink_abstractions::Variant;
use edgelink_abstractions::{engine::*, EdgeLinkError, Error, Result};

#[derive(Debug, serde::Deserialize)]
struct FlowConfig {
    id: String,
    label: String,
    disabled: bool,
    info: String,
}

pub struct Flow {
    pub id: u64,
    pub label: String,
    pub disabled: bool,
    pub nodes: Arc<Mutex<Vec<Box<dyn FlowNodeBehavior>>>>,
    pub context: Mutex<RefCell<Variant>>,
}

impl Flow {
    pub fn new(
        flow_elem: &serde_json::Value,
        elements: &Vec<serde_json::Value>,
    ) -> anyhow::Result<Self> {
        let flow_config: FlowConfig = serde_json::from_value(flow_elem.clone())?;

        println!(
            "-- Loading flow (id={0}, label='{1}'):",
            flow_config.id, flow_config.label
        );

        let u64_id = u64::from_str_radix(&flow_config.id, 16)?;

        let mut ctx_map = BTreeMap::new();
        ctx_map.insert("id".to_string(), Variant::String(flow_config.id));
        ctx_map.insert(
            "label".to_string(),
            Variant::String(flow_config.label.clone()),
        );

        for bnd in inventory::iter::<BuiltinNodeDescriptor> {
            println!("-- kind={}, type-name={}", bnd.kind, bnd.type_name);
        }

        Ok(Flow {
            id: u64_id,
            label: flow_config.label.clone(),
            disabled: flow_config.disabled,
            nodes: Arc::new(Mutex::new(Vec::new())),
            context: Mutex::new(RefCell::new(Variant::Object(ctx_map))),
        })
    }

    fn nodes(&self) -> Arc<Mutex<Vec<Box<dyn FlowNodeBehavior>>>> {
        self.nodes.clone()
    }

    async fn start(&self) {
        println!("Starting Flow (id={0})...", self.id);
        time::sleep(tokio::time::Duration::from_millis(100)).await;
    }

    async fn stop(&self) {
        time::sleep(tokio::time::Duration::from_millis(100)).await;
        println!("Stopping Flow (id={0})...", self.id);
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

    async fn start(&mut self) {
        println!("Starting Flow (id={0})...", self.id);
    }

    async fn stop(&mut self) {
        println!("Starting Flow (id={0})...", self.id);
    }
}

pub struct FlowEngineState {
    pub flows: edgelink_abstractions::engine::Flows,
}

impl FlowEngineState {
    pub fn new(flows_json_path: &str) -> Result<Self> {
        let mut file = File::open(flows_json_path)?;
        let mut contents = String::new();
        file.read_to_string(&mut contents)?;

        let mut flows = Flows::new();
        let json_value: serde_json::Value = serde_json::from_str(&contents)?;
        if let Some(elements) = json_value.as_array() {
            for e in elements.iter() {
                if let Some(item_type) = e["type"].as_str() {
                    if item_type == "tab" {
                        // let flow = &Flow::new(&e, &elements)? as &dyn FlowBehavior;
                        let flow = Flow::new(&e, &elements)?;
                        flows.push(Arc::new(Mutex::new(Box::new(flow))));
                    }
                }
            }
        }

        Ok(FlowEngineState { flows: flows })
    }
}

#[async_trait]
impl FlowEngine for FlowEngineState {
    fn get_flows(&self) -> &Flows {
        &self.flows
    }

    fn get_flows_mut(&mut self) -> &mut Flows {
        &mut self.flows
    }

    async fn start(&mut self) {
        let mut flows = self.get_flows_mut();
        for flow in flows {
            let flow_clone = flow.clone();
            let mut locked_flow = flow_clone.lock().unwrap();
            println!(
                "Starting: Flow(id={0}, label='{1}')",
                locked_flow.id(),
                locked_flow.label()
            );
        }
    }

    async fn stop(&mut self) {}
}
