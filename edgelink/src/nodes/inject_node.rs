use crate::flow::Flow;
use crate::nodes::*;
use crate::{nodes::*, red::json::RedFlowNodeConfig, Result};
use std::sync::{Arc, Weak};
use tokio::sync::Mutex;

struct InjectNode {
    base: BaseNode,
    flow: Weak<Flow>,
}

#[async_trait]
impl NodeBehavior for InjectNode {
    fn id(&self) -> u64 {
        self.base.id
    }

    async fn start(&self) -> Result<()> {
        // tokio::spawn(async move { });
        tokio::spawn(async move {
            println!("start delay");
            tokio::time::sleep(std::time::Duration::from_secs(2)).await;
            println!("end delay");
        });

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
        flow: Arc::downgrade(&flow),
    };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node))
}
