use std::sync::{Arc, Weak};
use std::time::Duration;

use crate::Result;
use crate::EdgeLinkError;
use crate::runtime::model::*;
use crate::runtime::nodes::*;
use crate::runtime::flow::Flow;
use crate::runtime::model::*;

struct InjectNode {
    base: Arc<BaseFlowNode>,
}

impl InjectNode {
    async fn cron_task(&self, cancel: CancellationToken) {
        while !cancel.is_cancelled() {
            // TODO FIXME
            let delay_result =
                crate::utils::async_util::delay(Duration::from_secs(2), cancel.child_token()).await;
            match delay_result {
                Ok(()) => {
                    let flow_ref = Weak::upgrade(&self.base().flow).unwrap();
                    let now = crate::utils::time::unix_now().unwrap();
                    let payload = Variant::from(now);
                    let msg = Msg::with_payload(self.base.id, payload);
                    flow_ref
                        .fan_out_single_port(self.base.id, 0, &[msg], cancel.clone())
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

    async fn start(&self, _cancel: CancellationToken) -> crate::Result<()> {
        Ok(())
    }

    async fn stop(&self) -> Result<()> {
        /*
            let cron_task_wrapper_ptr = self.cron_task_wrapper.clone();
            tokio::task::spawn(async move {
                let mut cron_task_wrapper = cron_task_wrapper_ptr.write().await;
                if let Some(cron_task) = cron_task_wrapper.task_handle {
                    cron_task.await;
                }
            })
            .await;
        */
        Ok(())
    }
}

#[async_trait]
impl FlowNodeBehavior for InjectNode {
    fn base(&self) -> &BaseFlowNode {
        &self.base
    }

    async fn process(&self, cancel: CancellationToken) {
        self.cron_task(cancel).await;
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
