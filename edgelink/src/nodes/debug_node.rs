use crate::flow::Flow;
use crate::nodes::*;
use crate::{nodes::*, red::json::RedFlowNodeConfig, Result};
use std::sync::{Arc, Weak};
use tokio::sync::Mutex;

struct DebugNode {
    base: BaseNode,
    flow: Weak<Flow>,
}

#[async_trait]
impl NodeBehavior for DebugNode {
    fn id(&self) -> ElementID {
        self.base.id
    }

    async fn start(&self) -> Result<()> {
        Ok(())
    }

    async fn stop(&self) -> Result<()> {
        Ok(())
    }
}

impl FlowNodeBehavior for DebugNode {}

fn new_node(flow: Arc<Flow>, config: &RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior> {
    let node = DebugNode {
        base: BaseNode {
            id: config.id,
            name: config.name.clone(),
        },
        flow: Arc::downgrade(&flow),
    };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "debug", NodeFactory::Flow(new_node))
}
