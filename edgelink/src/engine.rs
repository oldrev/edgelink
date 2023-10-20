use std::sync::Arc;
use tokio::sync::RwLock as TokRwLock;
use tokio_util::sync::CancellationToken;

use crate::flow::Flow;
use crate::nodes::{NodeBehavior, NodeFactory};
use crate::{registry::Registry, variant::Variant, EdgeLinkError};

struct FlowEngineState {
    flows: Vec<Arc<Flow>>,
    global_nodes: Vec<Arc<dyn NodeBehavior>>,
    _context: Variant,
    _shutdown: bool,
}

struct FlowEngineShared {
    state: TokRwLock<FlowEngineState>,
}

pub struct FlowEngine {
    shared: Arc<FlowEngineShared>,
}

impl FlowEngine {
    pub async fn new(
        reg: Arc<dyn Registry>,
        flows_json_path: &str,
    ) -> crate::Result<Arc<FlowEngine>> {
        let json_values = crate::red::json::load_flows_json(flows_json_path)?;

        let engine = Arc::new(FlowEngine {
            shared: Arc::new(FlowEngineShared {
                state: TokRwLock::new(FlowEngineState {
                    flows: Vec::new(),
                    global_nodes: Vec::new(),
                    _context: Variant::empty_object(),
                    _shutdown: false,
                }),
            }),
        });

        {
            let mut state = engine.shared.state.write().await;
            // load flows
            for flow_config in json_values.flows.iter() {
                let flow = Flow::new(engine.clone(), flow_config, reg.clone()).await?;
                state.flows.push(flow);
            }

            for global_config in json_values.global_nodes.iter() {
                if let Some(meta_node) = reg.get(global_config.type_name.as_str()) {
                    let node = match meta_node.factory {
                        NodeFactory::Global(factory) => factory(engine.clone(), global_config),
                        _ => {
                            return Err(EdgeLinkError::NotSupported(format!(
                                "Can not found global node factory for Node(id={0}, type='{1}'",
                                global_config.id, global_config.type_name
                            ))
                            .into())
                        }
                    };
                    state.global_nodes.push(node);
                }
            }
        }

        Ok(engine)
    }

    pub async fn start(&self, cancel: CancellationToken) -> crate::Result<()> {
        let state = self.shared.state.write().await;
        for flow in state.flows.iter() {
            //let flow_lock = TokMutex::new(flow.clone());
            let flow_lock = flow.clone();
            let child_cancel = cancel.clone();
            tokio::task::spawn(async move {
                let scoped_flow = flow_lock;
                scoped_flow.start(child_cancel).await
            })
            .await??;
        }
        println!("All flows started.");
        Ok(())
    }

    pub async fn stop(&self, cancel: CancellationToken) -> crate::Result<()> {
        let state = self.shared.state.write().await;
        for flow in state.flows.iter() {
            flow.clone().stop(cancel.clone()).await?;
        }
        Ok(())
    }
}
