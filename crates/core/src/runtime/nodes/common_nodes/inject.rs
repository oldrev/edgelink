use std::collections::BTreeMap;
use std::sync::Arc;
use std::time::Duration;

use serde::Deserialize;
use serde_json::Value;
use tokio_cron_scheduler::{Job, JobScheduler};

use crate::runtime::eval;
use crate::runtime::model::*;
use crate::runtime::nodes::*;
use edgelink_macro::*;

// const USER_INJECT_PROPS: &str = "__user_inject_props__";

#[derive(Debug, Clone, Deserialize)]
struct RedPropertyTriple {
    p: String,

    #[serde(default)]
    vt: RedPropertyType,

    #[serde(default)]
    v: String,
}

#[derive(serde::Deserialize, Debug)]
struct InjectNodeConfig {
    #[serde(default)]
    props: Vec<RedPropertyTriple>,

    #[serde(default, deserialize_with = "json::deser::str_to_option_f64")]
    repeat: Option<f64>,

    #[serde(default)]
    crontab: String,

    #[serde(default)]
    once: bool,

    #[serde(rename = "onceDelay", default)]
    once_delay: Option<f64>,
}

#[derive(Debug)]
#[flow_node("inject")]
struct InjectNode {
    base: FlowNode,
    config: InjectNodeConfig,
}

impl InjectNode {
    fn build(
        _flow: &Flow,
        base_node: FlowNode,
        _config: &RedFlowNodeConfig,
    ) -> crate::Result<Box<dyn FlowNodeBehavior>> {
        let json = handle_legacy_json(&_config.json);
        let mut inject_node_config = InjectNodeConfig::deserialize(&json)?;

        // fix the `crontab`
        if !inject_node_config.crontab.is_empty() {
            if inject_node_config.crontab.split_whitespace().count() == 6 {
                inject_node_config.crontab = inject_node_config.crontab.to_string();
            } else {
                inject_node_config.crontab = format!("0 {}", inject_node_config.crontab);
            }
        }

        let node = InjectNode { base: base_node, config: inject_node_config };
        Ok(Box::new(node))
    }

    async fn once_task(&self, stop_token: CancellationToken) -> crate::Result<()> {
        if let Some(once_delay_value) = self.config.once_delay {
            crate::utils::async_util::delay(Duration::from_secs_f64(once_delay_value), stop_token.clone()).await?;
        }

        self.inject_msg(stop_token).await?;
        Ok(())
    }

    async fn cron_task(self: Arc<Self>, stop_token: CancellationToken) -> crate::Result<()> {
        let mut sched = JobScheduler::new().await.unwrap_or_else(|e| {
            log::error!("Failed to create JobScheduler: {}", e);
            panic!("Failed to create JobScheduler")
        });

        if self.config.crontab.is_empty() {
            log::error!("Cron expression is missing");
            return Err(EdgelinkError::BadFlowsJson("Cron expression is missing".to_string()).into());
        }

        log::debug!("cron_expr='{}'", &self.config.crontab);

        let cron_job_stop_token = stop_token.clone();
        let self1 = Arc::clone(&self);

        let cron_job_result = Job::new_async(self.config.crontab.as_ref(), move |_, _| {
            let self2 = Arc::clone(&self1);
            let job_stop_token = cron_job_stop_token.clone();
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
                log::error!("Failed to parse cron: '{}' [node.name='{}']: {}", self.config.crontab, self.name(), e);
                return Err(e.into());
            }
        }

        log::info!("The CRON task has been stopped.");
        Ok(())
    }

    async fn repeat_task(&self, repeat_interval: f64, stop_token: CancellationToken) -> crate::Result<()> {
        while !stop_token.is_cancelled() {
            crate::utils::async_util::delay(Duration::from_secs_f64(repeat_interval), stop_token.clone()).await?;
            self.inject_msg(stop_token.clone()).await?;
        }
        log::info!("The `repeat` task has been stopped.");
        Ok(())
    }

    async fn inject_msg(&self, stop_token: CancellationToken) -> crate::Result<()> {
        // TODO msg.field1 references msg.field2
        let mut msg_body: BTreeMap<String, Variant> = BTreeMap::new();
        for prop in self.config.props.iter() {
            let k = prop.p.to_string();
            let v =
                eval::evaluate_node_property(&prop.v, prop.vt, Some(self), self.get_flow().upgrade().as_ref(), None)
                    .await?;
            msg_body.insert(k, v);
        }
        msg_body.insert(wellknown::MSG_ID_PROPERTY.to_string(), Variant::String(Msg::generate_id().to_string()));

        let envelope = Envelope { port: 0, msg: Msg::new_with_body(msg_body) };

        {
            let to_notify = envelope.msg.read().await;
            self.notify_uow_completed(&to_notify, stop_token.clone()).await;
        }

        self.fan_out_one(&envelope, stop_token.clone()).await
    }
}

#[async_trait]
impl FlowNodeBehavior for InjectNode {
    fn get_node(&self) -> &FlowNode {
        &self.base
    }

    async fn run(self: Arc<Self>, stop_token: CancellationToken) {
        let mut is_executed = false;
        if self.config.once {
            is_executed = true;
            if let Err(e) = self.once_task(stop_token.child_token()).await {
                log::warn!("The 'once_task' failed: {}", e.to_string());
            }
        }

        if let Some(repeat_interval) = self.config.repeat {
            is_executed = true;
            if let Err(e) = self.repeat_task(repeat_interval, stop_token.child_token()).await {
                log::warn!("The 'repeat_task' failed: {}", e.to_string());
            }
        } else if !self.config.crontab.is_empty() {
            is_executed = true;
            if let Err(e) = self.clone().cron_task(stop_token.child_token()).await {
                log::warn!("The CRON task failed: {}", e.to_string());
            }
        }

        if !is_executed {
            log::warn!("The InjectNode(id='{}', name='{}') has no trigger.", self.id(), self.name());
            stop_token.cancelled().await;
        }
    }
}

fn handle_legacy_json(orig: &Value) -> Value {
    let mut n = orig.clone();
    if let Value::Object(ref mut map) = n {
        if let Some(props) = map.get_mut("props") {
            if let Value::Array(ref mut props_array) = props {
                for prop in props_array {
                    if let Value::Object(ref mut prop_map) = prop {
                        if let Some(p) = prop_map.get("p") {
                            if p == "payload" && !prop_map.contains_key("v") {
                                prop_map.insert("v".to_string(), orig["payload"].clone());
                                prop_map.insert("vt".to_string(), orig["payloadType"].clone());
                            } else if p == "topic"
                                && prop_map.get("vt") == Some(&Value::String("str".to_string()))
                                && !prop_map.contains_key("v")
                            {
                                prop_map.insert("v".to_string(), orig["topic"].clone());
                            }
                        }
                    }
                }
            }
        } else {
            let mut new_props = Vec::new();
            new_props.push(serde_json::json!({
                "p": "payload",
                "v": orig["payload"],
                "vt": orig["payloadType"]
            }));
            new_props.push(serde_json::json!({
                "p": "topic",
                "v": orig["topic"],
                "vt": "str"
            }));
            map.insert("props".to_string(), Value::Array(new_props));
        }
    }
    n
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_red_property_triple_should_be_ok() {
        let data = r#"
        [{
            "p": "timestamp",
            "v": "",
            "vt": "date"
        }]
        "#;

        // Parse the string of data into serde_json::Value.
        let v: serde_json::Value = serde_json::from_str(data).unwrap();
        let triples = Vec::<RedPropertyTriple>::deserialize(&v).unwrap();
        assert_eq!(1, triples.len());
        assert_eq!("timestamp", triples[0].p);
        assert_eq!(RedPropertyType::Date, triples[0].vt);
    }
}
