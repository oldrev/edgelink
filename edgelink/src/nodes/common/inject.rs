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
    cron_task_wrapper: Arc<TokRwLock<CronTaskWrapper>>,
}

struct CronTaskWrapper {
    //task_handle: Option<tokio::task::JoinHandle<Result<(), tokio::task::JoinError>>>,
    task_handle: Option<tokio::task::JoinHandle<()>>,
}

/*
impl InjectNode {
    async fn cron_task(&self, _cancel: CancellationToken) {
        /*
        let flow_ptr = Weak::upgrade(&self.info.flow).unwrap();
        let self_id = self.info.id;
        let child_cancel = cancel.clone();
        */
    }
}
*/

#[async_trait]
impl NodeBehavior for InjectNode {
    fn id(&self) -> ElementId {
        self.base.id
    }

    fn name(&self) -> &str {
        &self.base.name
    }

    async fn start(&self, cancel: CancellationToken) -> crate::Result<()> {
        let cron_task_wrapper_ptr = self.cron_task_wrapper.clone();
        let mut cron_task_wrapper = cron_task_wrapper_ptr.write().await;
        let child_cancel = cancel.clone();
        let flow_ptr = Weak::upgrade(&self.base.flow).unwrap();
        let self_id = self.id();

        cron_task_wrapper.task_handle = Some(tokio::task::spawn(async move {
            while !child_cancel.is_cancelled() {
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
                        let now = crate::utils::time::unix_now().unwrap();
                        let payload = Variant::from(now);
                        let msg = Msg::with_payload(self_id, payload);
                        flow_ptr
                            .fan_out_single_port(self_id, 0, vec![msg], child_cancel.clone())
                            .await
                            .unwrap();
                    }
                    Err(_) => break,
                }
            }
            println!("The CRON task has been stopped.");
        }));

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
    fn ports(&self) -> &Vec<Port> {
        &self.base.ports
    }

    async fn process(&mut self, cancel: CancellationToken) -> crate::Result<()> {
        while !cancel.is_cancelled() {
            let msg = self.base.msg_receiver.recv().await.unwrap().clone();
            println!("收到消息：\n{:#?}", msg.as_ref());
        }
        Ok(())
    }
}

fn new_node(
    flow: Arc<Flow>,
    base_node: BaseFlowNode,
    config: &RedFlowNodeConfig,
) -> Box<dyn FlowNodeBehavior> {
    let node = InjectNode {
        base: base_node,
        cron_task_wrapper: Arc::new(TokRwLock::new(CronTaskWrapper { task_handle: None })),
    };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node))
}
