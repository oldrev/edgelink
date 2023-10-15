use async_trait::async_trait;
use log;
use std::collections::BTreeMap;
use std::future::Future;
use std::sync::Arc;
use std::thread::spawn;
use tokio::sync::broadcast;
use tokio::sync::mpsc;
use tokio::sync::Mutex as TokMutex;

use crate::flow::Flow;
use crate::nodes::{NodeBehavior, NodeFactory};
use crate::{registry::Registry, variant::Variant, EdgeLinkError, Result};

struct FlowEngineState {
    flows: Vec<Arc<Flow>>,
    global_nodes: Vec<Box<dyn NodeBehavior>>,
    context: Variant,
    shutdown: bool,
}

struct FlowEngineShared {
    state: TokMutex<FlowEngineState>,
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
                state: TokMutex::new(FlowEngineState {
                    flows: Vec::new(),
                    global_nodes: Vec::new(),
                    context: Variant::empty_object(),
                    shutdown: false,
                }),
            }),
        });

        {
            let mut state = engine.shared.state.lock().await;
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
                            return Err(EdgeLinkError::NotSupported(
                                format!(
                                    "Can not found global node factory for Node(id={0}, type='{1}'",
                                    global_config.id, global_config.type_name
                                )
                                .to_string(),
                            )
                            .into())
                        }
                    };
                    state.global_nodes.push(node);
                }
            }
        }

        Ok(engine)
    }

    pub async fn start(&self) -> crate::Result<()> {
        let state = self.shared.state.lock().await;
        for flow in state.flows.iter() {
            //let flow_lock = TokMutex::new(flow.clone());
            let flow_lock = flow.clone();
            tokio::spawn(async move {
                let scoped_flow = flow_lock;
                scoped_flow.start().await
            })
            .await??;
        }
        println!("All flows started.");
        Ok(())
    }

    pub async fn stop(&self) -> crate::Result<()> {
        let state = self.shared.state.lock().await;
        for flow in state.flows.iter() {
            flow.stop().await?;
        }
        Ok(())
    }
}

/*
/// Run the working loop, will not return unless a shotdown signal has been sent
pub async fn run(shutdown: impl Future) -> Result<()> {
    let (notify_shutdown, _) = broadcast::channel(1);
    let (shutdown_complete_tx, mut shutdown_complete_rx) = mpsc::channel(1);

    let reg = Registry::new()?;
    let engine = Arc::new(TokMutex::new(FlowEngine::new(&reg, "./flows.json").await?));
    tokio::spawn(async move {
        //
        let locked = engine.lock().await;
        locked.start().await
    })
    .await?;

    tokio::select! {
        res = engine.run() => {
            // If an error is received here, accepting connections from the TCP
            // listener failed multiple times and the server is giving up and
            // shutting down.
            //
            // Errors encountered when handling individual connections do not
            // bubble up to this point.
            if let Err(err) = res {
                error!(cause = %err, "failed to accept");
            }
        }
        _ = shutdown => {
            // The shutdown signal has been received.
            info!("shutting down");
        }
    }
}

*/
