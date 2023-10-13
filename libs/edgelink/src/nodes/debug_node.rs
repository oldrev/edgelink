use crate::nodes::*;
use edgelink_abstractions::nodes::*;

/*
struct DebugNode {
    base: BaseNode,
}

#[async_trait]
impl NodeBehavior for DebugNode {
    async fn start(&mut self) {}

    async fn stop(&mut self) {}
}

impl FlowNodeBehavior for DebugNode {

}

fn new_node(value: serde_json::Value) -> Box<dyn FlowNodeBehavior> {
    let node = DebugNode {
        base: BaseNode::from_json_value(&value),
    };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "debug", NodeFactory::Flow(new_node))
}

*/
