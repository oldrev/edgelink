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
    println!("我的爹是：{0}", node.flow.upgrade().unwrap().id());
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node))
}
