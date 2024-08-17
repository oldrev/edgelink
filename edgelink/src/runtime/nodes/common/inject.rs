use std::collections::BTreeMap;
use std::sync::Arc;
use std::time::Duration;

use log;
use tokio_cron_scheduler::{Job, JobScheduler};

use crate::runtime::flow::Flow;
use crate::runtime::model::*;
use crate::runtime::nodes::*;
use crate::runtime::red::eval;
use crate::runtime::red::json::RedPropertyTriple;
use crate::runtime::red::json::RedPropertyType;

struct InjectNode {
    base: Arc<BaseFlowNode>,

    repeat: Option<f64>,
    cron: Option<String>,
    once: bool,
    once_delay: Option<f64>,
    props: Vec<RedPropertyTriple>,
}

impl InjectNode {
    async fn once_task(&self, stop_token: CancellationToken) -> crate::Result<()> {
        if let Some(once_delay_value) = self.once_delay {
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
        let mut sched = JobScheduler::new().await.unwrap_or_else(|e| {
            log::error!("Failed to create JobScheduler: {}", e);
            panic!("Failed to create JobScheduler")
        });

        let cron_expr = match self.cron.as_ref() {
            Some(expr) => expr.as_ref(),
            None => {
                log::error!("Cron expression is missing");
                return Err("Cron expression is missing".into());
            }
        };

        log::debug!("cron_expr='{}'", cron_expr);

        let cron_job_stop_token = stop_token.child_token();
        let self1 = Arc::clone(&self);

        let cron_job_result = Job::new_async(cron_expr, move |_, _| {
            let self2 = Arc::clone(&self1);
            let job_stop_token = cron_job_stop_token.child_token();
            Box::pin(async move {
                if let Err(e) = self2.inject_msg(job_stop_token).await {
                    log::error!("Failed to inject: {}", e);
                }
            })
        });

        match cron_job_result {
            Ok(checked_job) => {
                sched.add(checked_job).await.unwrap_or_else(|e| {
                    log::error!("Failed to add job: {}", e);
                    panic!("Failed to add job")
                });

                sched.start().await.unwrap_or_else(|e| {
                    log::error!("Failed to start scheduler: {}", e);
                    panic!("Failed to start scheduler")
                });

                stop_token.cancelled().await;

                sched.shutdown().await.unwrap_or_else(|e| {
                    log::error!("Failed to shutdown scheduler: {}", e);
                    panic!("Failed to shutdown scheduler")
                });
            }
            Err(e) => {
                log::error!(
                    "Failed to parse cron: '{}' [node.name='{}']: {}",
                    cron_expr,
                    self.name(),
                    e
                );
                return Err(e.into());
            }
        }

        log::info!("The CRON task has been stopped.");
        Ok(())
    }

    async fn repeat_task(&self, stop_token: CancellationToken) -> crate::Result<()> {
        while !stop_token.is_cancelled() {
            crate::utils::async_util::delay(
                Duration::from_secs_f64(self.repeat.unwrap()),
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

        let msg_body: BTreeMap<String, Variant> = self
            .props
            .iter()
            .map(|i| {
                (
                    i.p.to_string(),
                    eval::evaluate_node_property(&i.v, &i.vt, self, None).unwrap(),
                )
            })
            .collect();

        let msg = Msg::new_with_body(self.base.id, msg_body);

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
        if self.once {
            if let Err(e) = self.once_task(stop_token.child_token()).await {
                log::error!("The 'once_task' failed: {}", e.to_string());
            }
        }

        if self.repeat.is_some() {
            if let Err(e) = self.repeat_task(stop_token.child_token()).await {
                log::error!("The 'repeat_task' failed: {}", e.to_string());
            }
        } else if self.cron.is_some() {
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
    let mut props = RedPropertyTriple::collection_from_json_value(
        &_config
            .json
            .get("props")
            .ok_or(EdgeLinkError::BadFlowsJson())
            .cloned()?,
    )?;

    if let Some(payload_type) = _config.json.get("payloadType").and_then(|v| v.as_str()) {
        props.retain(|x| x.p != "payload");
        props.push(RedPropertyTriple {
            p: "payload".to_string(),
            vt: RedPropertyType::from(payload_type)?,
            v: "string".to_string(),
        });
    }

    let node = InjectNode {
        base: base_node,

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

        props: props,
    };
    Ok(Arc::new(node))
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node))
}
