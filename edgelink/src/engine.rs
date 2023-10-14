use async_trait::async_trait;
use std::collections::BTreeMap;
use std::sync::Arc;
use tokio::sync::Mutex;

use crate::flow::Flow;
use edgelink_abstractions::red::JsonValues;
use edgelink_abstractions::Variant;
use edgelink_abstractions::{engine::*, EdgeLinkError, Registry, Result};

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
        let json_values = crate::red::json::load_flows_json(flows_json_path)?;

        // load flows
        let mut flows = Vec::new();
        for e in json_values.flows.iter() {
            let flow = Flow::new(&e, &json_values)?;
            flows.push(Box::new(flow));
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

    pub async fn start(&mut self) -> Result<()> {
        let mut state = self.shared.state.lock().await;
        for flow in state.flows.iter_mut() {
            flow.start().await;
        }
        Ok(())
    }

    pub async fn stop(&mut self) -> Result<()> {
        let mut state = self.shared.state.lock().await;
        for flow in state.flows.iter_mut() {
            flow.stop().await;
        }
        Ok(())
    }
}

#[async_trait(?Send)]
impl FlowEngineBehavior for FlowEngine {}
