use std::sync::Arc;
use std::time::Duration;

use log;
use tokio_cron_scheduler::{Job, JobScheduler};

use crate::runtime::flow::Flow;
use crate::runtime::model::*;
use crate::runtime::nodes::*;

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
    async fn once_task(&self, stop_token: CancellationToken) -> crate::Result<()> {
        if let Some(once_delay_value) = self.config.once_delay {
            crate::utils::async_util::delay(
                Duration::from_secs_f64(once_delay_value),
                stop_token.child_token(),
            )
            .await?;
        }

        self.inject_msg(stop_token).await?;
        Ok(())
    }

    async fn cron_task(self: Arc<Self>, stop_token: CancellationToken) -> crate::Result<()> {
        let mut sched = JobScheduler::new().await.unwrap();

        let self1 = Arc::clone(&self);
        // Add async job
        let cron_job_stop_token = stop_token.child_token();
        let cron_expr = self.config.cron.as_ref().unwrap().as_ref();
        log::debug!("cron_expr='{}'", cron_expr);
        let cron_job_result = Job::new_async(cron_expr, move |_, _| {
            let self2 = Arc::clone(&self1);
            Box::pin({
                let job_stop_token = cron_job_stop_token.child_token();
                async move {
                    if let Err(e) = self2.inject_msg(job_stop_token.child_token()).await {
                        log::error!("Failed to inject: {}", e.to_string());
                    }
                }
            })
        });
        match cron_job_result {
            Ok(checked_job) => {
                sched.add(checked_job).await.unwrap();

                sched.start().await.unwrap();

                stop_token.cancelled().await;

                sched.shutdown().await.unwrap();
            }
            Err(e) => {
                log::error!(
                    "Failed to parse cron: '{}' [node.name='{}']: {}",
                    cron_expr,
                    self.name(),
                    e
                );
                panic!("Failed to parse cron"); //FIXME
            }
        }

        log::info!("The CRON task has been stopped.");
        Ok(())
    }

    async fn repeat_task(&self, stop_token: CancellationToken) -> crate::Result<()> {
        while !stop_token.is_cancelled() {
            crate::utils::async_util::delay(
                Duration::from_secs_f64(self.config.repeat.unwrap()),
                stop_token.child_token(),
            )
            .await?;
            self.inject_msg(stop_token.child_token()).await?;
        }
        log::info!("The `repeat` task has been stopped.");
        Ok(())
    }

    async fn inject_msg(&self, stop_token: CancellationToken) -> crate::Result<()> {
        let flow_ref = Weak::upgrade(&self.base().flow).unwrap();
        let now = crate::utils::time::unix_now().unwrap();
        let payload = Variant::from(now);
        let msg = Msg::new_with_payload(self.base.id, payload);
        flow_ref
            .fan_out_single_port(self.base.id, 0, &[msg], stop_token.clone())
            .await?;
        Ok(())
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
            if let Err(e) = self.once_task(stop_token.child_token()).await {
                log::error!("The 'once_task' failed: {}", e.to_string());
            }
        }

        if self.config.repeat.is_some() {
            if let Err(e) = self.repeat_task(stop_token.child_token()).await {
                log::error!("The 'repeat_task' failed: {}", e.to_string());
            }
        } else if self.config.cron.is_some() {
            if let Err(e) = self.clone().cron_task(stop_token.child_token()).await {
                log::error!("The CRON task failed: {}", e.to_string());
            }
        } else {
            log::warn!(
                "The inject node [id='{}', name='{}'] has no trigger.",
                self.base.id,
                self.base.name
            );
            stop_token.cancelled().await;
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
            .and_then(|v| {
                if v.is_empty() {
                    None
                } else {
                    Some(format!("0 {}", v))
                }
            }),

        once: _config.json.get("once").unwrap().as_bool().unwrap(),

        once_delay: _config
            .json
            .get("onceDelay")
            .and_then(|jv| jv.as_str())
            .and_then(|value| value.parse::<f64>().ok()),
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
