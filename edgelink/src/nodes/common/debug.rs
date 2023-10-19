use crate::flow::Flow;
use crate::nodes::*;
use crate::red::json::{RedFlowNodeConfig, RedPortConfig};
use crate::Result;
use std::borrow::BorrowMut;
use std::sync::Arc;

struct DebugNode {
    base: BaseFlowNode,
}

#[async_trait]
impl NodeBehavior for DebugNode {
    fn id(&self) -> ElementId {
        self.base.id
    }

    fn name(&self) -> &str {
        &self.base.name
    }

    async fn start(&self, _cancel: CancellationToken) -> Result<()> {
        Ok(())
    }

    async fn stop(&self, _cancel: CancellationToken) -> Result<()> {
        Ok(())
    }
}

#[async_trait]
impl FlowNodeBehavior for DebugNode {
    fn base(&self) -> &BaseFlowNode {
        &self.base
    }

    async fn process(&self, cancel: CancellationToken) {
        let mut recv_guard = self.base.msg_receiver.lock().await;
        while !cancel.is_cancelled() {
            if let Some(msg) = recv_guard.recv().await {
                println!("收到消息：\n{:#?}", msg.as_ref());
            } else {
                //break;
                println!("咋个会已关闭");
            }
        }
    }
}

fn new_node(
    flow: Arc<Flow>,
    base_node: BaseFlowNode,
    config: &RedFlowNodeConfig,
) -> Box<dyn FlowNodeBehavior> {
    let node = DebugNode { base: base_node };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "debug", NodeFactory::Flow(new_node))
}
