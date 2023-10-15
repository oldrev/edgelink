use crate::flow::Flow;
use crate::nodes::*;
use crate::{nodes::*, red::json::RedFlowNodeConfig, Result};
use std::sync::Arc;
use tokio::sync::Mutex;

struct InjectNode {
    base: BaseNode,
}

#[async_trait]
impl NodeBehavior for InjectNode {
    async fn start(&self) -> Result<()> {
        Ok(())
    }

    async fn stop(&self) -> Result<()> {
        Ok(())
    }
}

impl FlowNodeBehavior for InjectNode {}

fn new_node(flow: Arc<Flow>, config: &RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior> {
    let node = InjectNode {
        base: BaseNode {
            id: config.id,
            name: config.name.clone(),
        },
    };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node))
}
