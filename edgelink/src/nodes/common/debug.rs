use crate::flow::Flow;
use crate::nodes::*;
use crate::Result;
use crate::red::json::{RedFlowNodeConfig, PortConfig};
use std::sync::Arc;

struct DebugNode {
    info: FlowNodeInfo,
}

#[async_trait]
impl NodeBehavior for DebugNode {
    fn id(&self) -> ElementId {
        self.info.id
    }

    fn name(&self) -> &str {
        &self.info.name
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
    fn ports(&self) -> &Vec<PortConfig> {
        &self.info.ports
    }

    async fn fan_in(&self, msg: Arc<Msg>, _cancel: CancellationToken) -> crate::Result<()> {
        println!("收到消息：\n{:#?}", msg.as_ref());
        Ok(())
    }
}

fn new_node(flow: Arc<Flow>, config: &RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior> {
    let node = DebugNode {
        info: FlowNodeInfo {
            id: config.id,
            flow: Arc::downgrade(&flow),
            name: config.name.clone(),
            ports: config.wires.clone(),
        },
    };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "debug", NodeFactory::Flow(new_node))
}
