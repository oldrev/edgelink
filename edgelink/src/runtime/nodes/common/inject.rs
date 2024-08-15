use std::str::FromStr;
use std::sync::Arc;
use std::time::Duration;
use tokio::sync::Mutex as TokMutex;

use log;
use tokio_cron_scheduler::{Job, JobScheduler, JobSchedulerError};

use crate::runtime::flow::Flow;
use crate::runtime::model::*;
use crate::runtime::nodes::*;
use crate::EdgeLinkError;

#[derive(Debug, Clone)]
struct InjectNodeConfig {
    repeat: Option<f64>,
    cron: Option<String>,
    once: bool,
    once_delay: Option<f64>,
}

struct InjectNode {
    base: Arc<BaseFlowNode>,
    config: InjectNodeConfig,
}

impl InjectNode {
    fn create_msg(&self) -> Arc<Msg> {
        let now = crate::utils::time::unix_now().unwrap();
        let payload = Variant::from(now);
        Msg::with_payload(self.base.id, payload)
    }

    async fn once_task(&self, stop_token: CancellationToken) {
        let msg = self.create_msg();

        let flow_ref = Weak::upgrade(&self.base().flow).unwrap();
        flow_ref
            .fan_out_single_port(self.base.id, 0, &[msg], stop_token.child_token())
            .await
            .unwrap();
    }

    async fn cron_task(self: Arc<Self>, stop_token: CancellationToken) {
        let mut sched = JobScheduler::new().await.unwrap();

        let self1 = Arc::clone(&self);
        // Add async job
        let cron_job_stop_token = stop_token.child_token();
        let cron_expr = self.config.cron.as_ref().unwrap().as_ref();
        log::debug!("cron_expr='{}'", cron_expr);
        let cron_job = Job::new_async(cron_expr, move |_, _| {
            let self2 = Arc::clone(&self1);
            Box::pin({
                let job_stop_token = cron_job_stop_token.child_token();
                async move {
                    self2.node_action(job_stop_token.child_token()).await;
                }
            })
        });
        match cron_job {
            Ok(checked_job) => {
                sched.add(checked_job).await.unwrap();

                sched.start().await.unwrap();

                stop_token.cancelled().await;

                sched.shutdown().await.unwrap();
            }
            Err(e) => {
                log::error!(
                    "Failed to parse cron: '{}', node.name='{}'",
                    cron_expr,
                    self.name()
                );
                panic!("Failed to parse cron"); //FIXME
            }
        }

        log::info!("The CRON task has been stopped.");
    }

    async fn repeat_task(&self, stop_token: CancellationToken) {
        while !stop_token.is_cancelled() {
            // TODO FIXME
            let delay_result = crate::utils::async_util::delay(
                Duration::from_secs_f64(self.config.repeat.unwrap()),
                stop_token.child_token(),
            )
            .await;
            match delay_result {
                Ok(()) => {
                    self.node_action(stop_token.child_token()).await;
                }
                Err(ref err) => match err.downcast_ref().unwrap() {
                    EdgeLinkError::TaskCancelled => {
                        log::warn!("Inject task has been cancelled.");
                        break;
                    }
                    _ => break,
                },
            };
        }
        log::info!("The `repeat` task has been stopped.");
    }

    async fn node_action(&self, stop_token: CancellationToken) {
        let flow_ref = Weak::upgrade(&self.base().flow).unwrap();
        let now = crate::utils::time::unix_now().unwrap();
        let payload = Variant::from(now);
        let msg = Msg::with_payload(self.base.id, payload);
        flow_ref
            .fan_out_single_port(self.base.id, 0, &[msg], stop_token.clone())
            .await
            .unwrap(); //FIXME
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

    async fn run(self: Arc<Self>, stop_token: CancellationToken) {
        if self.config.once {
            self.once_task(stop_token.child_token()).await;
        }

        if self.config.repeat.is_some() {
            self.repeat_task(stop_token.child_token()).await;
        } else if self.config.cron.is_some() {
            self.clone().cron_task(stop_token.child_token()).await;
        } else {
            log::warn!("The inject node [{}] has no trigger.", self.base.name);
        }
    }
}

fn new_node(
    _flow: Arc<Flow>,
    base_node: Arc<BaseFlowNode>,
    _config: &RedFlowNodeConfig,
) -> crate::Result<Arc<dyn FlowNodeBehavior>> {
    let config = InjectNodeConfig {
        repeat: _config
            .json
            .get("repeat")
            .and_then(|jv| jv.as_str())
            .and_then(|value| value.parse::<f64>().ok()),
        cron: _config
            .json
            .get("crontab")
            .and_then(|v| v.as_str())
            .and_then(|v| Some(format!("0 {}", v))),

        once: _config.json.get("once").unwrap().as_bool().unwrap(),
        once_delay: _config.json.get("onceDelay").unwrap().as_f64(),
    };

    let node = InjectNode {
        base: base_node,
        config,
    };
    Ok(Arc::new(node))
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node))
}
