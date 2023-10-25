use std::sync::Arc;

use crate::Result;
use crate::runtime::flow::Flow;
use crate::runtime::nodes::*;
use crate::runtime::red::json::RedFlowNodeConfig;
use crate::runtime::model::*;

struct DebugNode {
    base: Arc<BaseFlowNode>,
}

#[async_trait]
impl NodeBehavior for DebugNode {
    fn id(&self) -> ElementId {
        self.base.id
    }

    fn name(&self) -> &str {
        &self.base.name
    }
}

#[async_trait]
impl FlowNodeBehavior for DebugNode {
    fn base(&self) -> &BaseFlowNode {
        &self.base
    }

    async fn run(&self, stop_token: CancellationToken) {
        while !stop_token.is_cancelled() {
            match self.wait_for_msg(stop_token.clone()).await {
                Ok(msg) => println!("收到消息：\n{:#?}", msg.as_ref()),
                Err(ref err) => {
                    println!("Error: \n{:#?}", err);
                    break;
                }
            }
        }

        let rx = &mut self.base().msg_rx.rx.lock().await;
        rx.close();
        println!("DebugNode process() task has been terminated.");
    }
}

fn new_node(
    _flow: Arc<Flow>,
    base_node: Arc<BaseFlowNode>,
    _config: &RedFlowNodeConfig,
) -> Arc<dyn FlowNodeBehavior> {
    let node = DebugNode { base: base_node };
    Arc::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "debug", NodeFactory::Flow(new_node))
}
