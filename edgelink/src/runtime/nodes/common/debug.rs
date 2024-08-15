use log;
use std::sync::Arc;

use crate::runtime::flow::Flow;
use crate::runtime::model::*;
use crate::runtime::nodes::*;
use crate::runtime::red::json::RedFlowNodeConfig;

struct DebugNode {
    base: Arc<BaseFlowNode>,
}

#[async_trait]
impl NodeBehavior for DebugNode {
    fn id(&self) -> ElementId {
        self.base.id
    }

    fn name(&self) -> &str {
        &self.base.name
    }
}

#[async_trait]
impl FlowNodeBehavior for DebugNode {
    fn base(&self) -> &BaseFlowNode {
        &self.base
    }

    async fn run(self: Arc<Self>, stop_token: CancellationToken) {
        while !stop_token.is_cancelled() {
            match self.wait_for_msg(stop_token.clone()).await {
                Ok(msg) => log::info!(
                    "Message Received [Node: {}] ：\n{:#?}",
                    self.name(),
                    msg.as_ref()
                ),
                Err(ref err) => {
                    log::error!("Error: \n{:#?}", err);
                    break;
                }
            }
        }

        let rx = &mut self.base().msg_rx.rx.lock().await;
        rx.close();
        log::debug!("DebugNode process() task has been terminated.");
    }
}

fn new_node(
    _flow: Arc<Flow>,
    base_node: Arc<BaseFlowNode>,
    _config: &RedFlowNodeConfig,
) -> crate::Result<Arc<dyn FlowNodeBehavior>> {
    let node = DebugNode { base: base_node };
    Ok(Arc::new(node))
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "debug", NodeFactory::Flow(new_node))
}
