use crate::flow::Flow;
use crate::nodes::*;
use crate::{nodes::*, red::json::RedFlowNodeConfig, Result};
use std::sync::{Arc, Weak};
use tokio::sync::Mutex;

struct DebugNode {
    base: BaseNode,
    flow: Weak<Flow>,
    ports: Vec<Port>,
}

#[async_trait]
impl NodeBehavior for DebugNode {
    fn id(&self) -> ElementID {
        self.base.id
    }

    fn name(&self) -> &str {
        &self.base.name
    }

    async fn start(&self) -> Result<()> {
        Ok(())
    }

    async fn stop(&self) -> Result<()> {
        Ok(())
    }
}

#[async_trait]
impl FlowNodeBehavior for DebugNode {
    fn ports(&self) -> &Vec<Port> {
        &self.ports
    }

    async fn fan_in(&self, msg: Arc<Msg>) -> crate::Result<()> {
        println!("收到消息：\n{:#?}", msg.clone().as_ref());
        Ok(())
    }
}

fn new_node(flow: Arc<Flow>, config: &RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior> {
    let node = DebugNode {
        base: BaseNode {
            id: config.id,
            name: config.name.clone(),
        },
        flow: Arc::downgrade(&flow),
        ports: config.wires.clone(),
    };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "debug", NodeFactory::Flow(new_node))
}
