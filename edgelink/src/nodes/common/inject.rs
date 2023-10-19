use crate::flow::Flow;
use crate::msg::Msg;
use crate::nodes::*;
use crate::red::json::*;
use crate::variant::Variant;
use crate::{EdgeLinkError, Result};
use std::sync::{Arc, Weak};
use std::time::Duration;
use tokio::sync::RwLock as TokRwLock;

struct InjectNode {
    base: BaseFlowNode,
}

impl InjectNode {
    async fn cron_task(&self, cancel: CancellationToken) {
        while !cancel.is_cancelled() {
            // TODO FIXME
            let delay_result =
                crate::async_util::delay(Duration::from_secs(1), cancel.clone()).await;
            tokio::select! {
                _ = cancel.cancelled() => {
                    // 取消 sleep_task 任务
                    println!("Cancelling the CRON task in inject node...");
                    break;
                }
                _ = tokio::time::sleep(Duration::from_secs(1)) => {
                    // Long work has completed
                }
            }
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
                Err(_) => todo!(),
            }
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

    async fn start(&self, cancel: CancellationToken) -> crate::Result<()> {
        Ok(())
    }

    async fn stop(&self, _cancel: CancellationToken) -> Result<()> {
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
        self.cron_task(cancel.clone()).await;
    }
}

fn new_node(
    flow: Arc<Flow>,
    base_node: BaseFlowNode,
    config: &RedFlowNodeConfig,
) -> Box<dyn FlowNodeBehavior> {
    let node = InjectNode { base: base_node };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node))
}
