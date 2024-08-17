use std::collections::BTreeMap;
use std::sync::Arc;


use crate::runtime::flow::Flow;
use crate::runtime::model::*;
use crate::runtime::nodes::*;

struct CompleteNode {
    base: Arc<BaseFlowNode>,
    //scope: BTreeMap<ElementId, Arc<dyn FlowNodeBehavior>>,
}

impl CompleteNode {
}

#[async_trait]
impl NodeBehavior for CompleteNode {
    fn id(&self) -> ElementId {
        self.base.id
    }

    fn name(&self) -> &str {
        &self.base.name
    }
}

#[async_trait]
impl FlowNodeBehavior for CompleteNode {
    fn base(&self) -> &BaseFlowNode {
        &self.base
    }

    async fn run(self: Arc<Self>, stop_token: CancellationToken) {
        stop_token.cancelled().await;
    }
}

fn new_node(
    _flow: Arc<Flow>,
    base_node: Arc<BaseFlowNode>,
    _config: &RedFlowNodeConfig,
) -> crate::Result<Arc<dyn FlowNodeBehavior>> {
    let node = CompleteNode {
        base: base_node,
        //scope: BTreeMap::new(),
    };
    Ok(Arc::new(node))
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "complete", NodeFactory::Flow(new_node))
}
