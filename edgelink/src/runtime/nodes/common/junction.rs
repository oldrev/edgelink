use std::sync::Arc;

use log;

use crate::runtime::flow::Flow;
use crate::runtime::model::*;
use crate::runtime::nodes::*;

struct JunctionNode {
    base: Arc<BaseFlowNode>,
}

#[async_trait]
impl NodeBehavior for JunctionNode {
    fn id(&self) -> ElementId {
        self.base.id
    }

    fn name(&self) -> &str {
        &self.base.name
    }
}

#[async_trait]
impl FlowNodeBehavior for JunctionNode {
    fn base(&self) -> &BaseFlowNode {
        &self.base
    }

    async fn run(self: Arc<Self>, stop_token: CancellationToken) {
        let flow_ref = Weak::upgrade(&self.base().flow).unwrap();
        while !stop_token.is_cancelled() {
            match self.wait_for_msg(stop_token.clone()).await {
                Ok(msg) => {
                    flow_ref
                        .fan_out_single_port(&self.base.id, 0, &[msg], stop_token.clone())
                        .await
                        .unwrap(); //FIXME
                }
                Err(ref err) => {
                    log::error!("Error: \n{:#?}", err);
                    break;
                }
            }
        }

        let rx = &mut self.base().msg_rx.rx.lock().await;
        rx.close();
        log::debug!("DebugNode process() task has been terminated.");
    }
}

fn new_node(
    _flow: Arc<Flow>,
    base_node: Arc<BaseFlowNode>,
    _config: &RedFlowNodeConfig,
) -> crate::Result<Arc<dyn FlowNodeBehavior>> {
    let node = JunctionNode {
        base: base_node,
    };
    Ok(Arc::new(node))
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "junction", NodeFactory::Flow(new_node))
}
