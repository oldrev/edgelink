use log;
use std::collections::BTreeMap;
use std::sync::Arc;
use tokio::sync::RwLock as TokRwLock;
use tokio::sync::Mutex as TokMutex;
use tokio_util::sync::CancellationToken;
use tokio::sync::mpsc;

use crate::runtime::model::Variant;
use crate::runtime::flow::Flow;
use crate::runtime::nodes::{NodeBehavior, NodeFactory};
use crate::runtime::registry::Registry;
use crate::EdgeLinkError;

use super::model::ElementId;

struct FlowEngineState {
    flows: BTreeMap<ElementId, Arc<Flow>>,
    global_nodes: BTreeMap<ElementId, Arc<dyn NodeBehavior>>,
    _context: Variant,
    _shutdown: bool,
}

struct FlowEngineShared {
    state: TokRwLock<FlowEngineState>,
}

pub struct FlowEngine {
    pub stopped_tx: mpsc::Sender<()>,
    stopped_rx: TokMutex<mpsc::Receiver<()>>,
    shared: Arc<FlowEngineShared>,
    stop_token: CancellationToken,
}

impl FlowEngine {
    pub async fn new(
        reg: Arc<dyn Registry>,
        flows_json_path: &str,
    ) -> crate::Result<Arc<FlowEngine>> {
        let json_values = crate::runtime::red::json::load_flows_json(flows_json_path)?;
        let (stopped_tx, mut stopped_rx) = mpsc::channel(1);

        let engine = Arc::new(FlowEngine {
            stopped_rx: TokMutex::new(stopped_rx),
            stopped_tx,
            stop_token: CancellationToken::new(),
            shared: Arc::new(FlowEngineShared {
                state: TokRwLock::new(FlowEngineState {
                    flows: BTreeMap::new(),
                    global_nodes: BTreeMap::new(),
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
                state.flows.insert(flow.id, flow);
            }

            for global_config in json_values.global_nodes.iter() {
                if let Some(meta_node) = reg.get(global_config.type_name.as_str()) {
                    let node = match meta_node.factory {
                        NodeFactory::Global(factory) => factory(engine.clone(), global_config)?,
                        _ => {
                            return Err(EdgeLinkError::NotSupported(format!(
                                "Can not found global node factory for Node(id={0}, type='{1}'",
                                global_config.id, global_config.type_name
                            ))
                            .into())
                        }
                    };
                    state.global_nodes.insert(node.id(), node);
                }
            }
        }

        Ok(engine)
    }

    pub async fn start(&self) -> crate::Result<()> {
        let state = self.shared.state.write().await;
        for flow in state.flows.values() {
            //let flow_lock = TokMutex::new(flow.clone());
            let flow_lock = flow.clone();
            tokio::task::spawn(async move {
                let scoped_flow = flow_lock;
                scoped_flow.start().await
            })
            .await??;
        }
        log::info!("All flows started.");
        Ok(())
    }

    pub async fn stop(&self) -> crate::Result<()> {
        log::info!("Stopping all flows...");
        self.stop_token.cancel();
        let state = self.shared.state.write().await;
        for flow in state.flows.values() {
            flow.clone().stop().await?;
        }
        //drop(&self.stopped_tx);
        let stopped_rx = &mut self.stopped_rx.lock().await;
        let _ = stopped_rx.recv().await;
        log::info!("All flows stopped.");
        Ok(())
    }
}
