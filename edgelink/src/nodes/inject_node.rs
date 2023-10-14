use crate::nodes::*;
use edgelink_abstractions::nodes::*;

/*
struct InjectNode {
    pub base: FlowNode,
}

#[async_trait]
impl NodeBehavior for InjectNode {
    async fn start(&mut self) {}

    async fn stop(&mut self) {}
}

impl FlowNodeBehavior for InjectNode {

}

fn new_node() -> Box<dyn FlowNodeBehavior> {
    let node = InjectNode {
        base: BaseNode::from_json_value(&value),
    };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node, &self))
}

*/
