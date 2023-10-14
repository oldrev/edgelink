use crate::nodes::*;
use edgelink_abstractions::{Result, nodes::*, red::RedFlowNodeConfig, engine::FlowBehavior};
use std::sync::{Arc};
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

impl FlowNodeBehavior for InjectNode {

    fn flow(&self) ->  &Box<dyn FlowBehavior>  {
        todo!()
    }

    fn flow_mut(&self) ->  &mut Box<dyn FlowBehavior>  {
        todo!()
    }
}

fn new_node(config: &RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior> {
    let node = InjectNode {
        base: BaseNode { id: config.id, name: config.name.clone() },
    };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node))
}