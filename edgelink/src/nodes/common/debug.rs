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
    fn ports(&self) -> &Vec<Port> {
        &self.base.ports
    }

    async fn process(&mut self, cancel: CancellationToken) -> crate::Result<()> {
        while !cancel.is_cancelled() {
            let msg = self.base.msg_receiver.recv().await.unwrap();
            println!("收到消息：\n{:#?}", msg.as_ref());
        }
        Ok(())
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
