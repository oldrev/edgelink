use crate::nodes::*;
use edgelink_abstractions::nodes::*;

struct InjectNode {
    pub base: FlowNode,
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject")
}
