use std::sync::{Arc, Weak};
use std::time::Duration;

use crate::runtime::flow::Flow;
use crate::runtime::model::*;
use crate::runtime::model::*;
use crate::runtime::nodes::*;
use crate::EdgeLinkError;
use crate::Result;

struct InjectNode {
    base: Arc<BaseFlowNode>,
}

impl InjectNode {
    async fn cron_task(&self, stop_token: CancellationToken) {
        while !stop_token.is_cancelled() {
            // TODO FIXME
            let delay_result = crate::utils::async_util::delay(
                Duration::from_secs(2),
                stop_token.child_token(),
            )
            .await;
            match delay_result {
                Ok(()) => {
                    let flow_ref = Weak::upgrade(&self.base().flow).unwrap();
                    let now = crate::utils::time::unix_now().unwrap();
                    let payload = Variant::from(now);
                    let msg = Msg::with_payload(self.base.id, payload);
                    flow_ref
                        .fan_out_single_port(self.base.id, 0, &[msg], stop_token.clone())
                        .await
                        .unwrap();
                }
                Err(ref err) => match err.downcast_ref().unwrap() {
                    EdgeLinkError::TaskCancelled => {
                        println!("Inject task has been cancelled.");
                        break;
                    }
                    _ => break,
                },
            };
        }
        println!("The CRON task has been stopped.");
    }
}

#[async_trait]
impl NodeBehavior for InjectNode {
    fn id(&self) -> ElementId {
        self.base.id
    }

    fn name(&self) -> &str {
        &self.base.name
    }

}

#[async_trait]
impl FlowNodeBehavior for InjectNode {
    fn base(&self) -> &BaseFlowNode {
        &self.base
    }

    async fn run(&self, stop_token: CancellationToken) {
        self.cron_task(stop_token).await;
    }
}

fn new_node(
    _flow: Arc<Flow>,
    base_node: Arc<BaseFlowNode>,
    _config: &RedFlowNodeConfig,
) -> Arc<dyn FlowNodeBehavior> {
    let node = InjectNode { base: base_node };
    Arc::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node))
}
