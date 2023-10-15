use async_trait::async_trait;
use std::collections::BTreeMap;
use std::sync::{Arc, Weak};
use tokio::sync::Mutex as TokMutex;

use crate::flow::Flow;
use crate::{registry::Registry, variant::Variant, EdgeLinkError, Result};

struct FlowEngineState {
    flows: Vec<Arc<Flow>>,
    context: Variant,
}

struct FlowEngineShared {
    state: TokMutex<FlowEngineState>,
}

pub struct FlowEngine {
    shared: Arc<FlowEngineShared>,
}

impl FlowEngine {
    pub async fn new(reg: &Registry, flows_json_path: &str) -> Result<Arc<FlowEngine>> {
        let json_values = crate::red::json::load_flows_json(flows_json_path)?;

        let engine = Arc::new(FlowEngine {
            shared: Arc::new(FlowEngineShared {
                state: TokMutex::new(FlowEngineState {
                    flows: Vec::new(),
                    context: Variant::Object(BTreeMap::new()),
                }),
            }),
        });

        {
            let mut state = engine.shared.state.lock().await;
            // load flows
            for flow_config in json_values.flows.iter() {
                let flow = Flow::new(engine.clone(), flow_config, reg).await?;
                state.flows.push(flow);
            }
        }

        Ok(engine)
    }

    pub async fn start(&self) -> Result<()> {
        let mut state = self.shared.state.lock().await;
        for flow in state.flows.iter_mut() {
            flow.start().await?;
        }
        Ok(())
    }

    pub async fn stop(&self) -> Result<()> {
        let mut state = self.shared.state.lock().await;
        for flow in state.flows.iter_mut() {
            flow.stop().await?;
        }
        Ok(())
    }
}