use crate::flow::Flow;
use crate::model::Port;
use crate::msg::Msg;
use crate::nodes::*;
use crate::{nodes::*, red::json::RedFlowNodeConfig, Result};
use crate::variant::Variant;
use std::sync::{Arc, Weak};
use std::time::{SystemTime, UNIX_EPOCH};
use tokio::sync::Mutex as TokMutex;

struct InjectNode {
    base: BaseNode,
    flow: Weak<Flow>,
    ports: Vec<Port>,
}

#[async_trait]
impl NodeBehavior for InjectNode {
    fn id(&self) -> ElementID {
        self.base.id
    }

    fn name(&self) -> &str {
        &self.base.name
    }

    async fn start(&self) -> Result<()> {
        let flow_ptr = Weak::upgrade(&self.flow).unwrap();
        let self_id = self.id();
        tokio::spawn(async move {
            loop {
                // TODO FIXME
                tokio::time::sleep(std::time::Duration::from_secs(2)).await;
                let now = unix_now().unwrap();
                let payload = Variant::from(now);
                let msg = Msg::with_payload(self_id,  payload);
                flow_ptr.fan_out(msg).await.unwrap();
            }
        });

        Ok(())
    }

    async fn stop(&self) -> Result<()> {
        Ok(())
    }
}

#[async_trait]
impl FlowNodeBehavior for InjectNode {
    fn ports(&self) -> &Vec<Port> {
        &self.ports
    }

    async fn fan_in(&self, msg: Arc<Msg>) -> crate::Result<()> {
        Err(EdgeLinkError::NotSupported("This node is a source node".to_string()).into())
    }
}

fn new_node(flow: Arc<Flow>, config: &RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior> {
    let node = InjectNode {
        base: BaseNode {
            id: config.id,
            name: config.name.clone(),
        },
        flow: Arc::downgrade(&flow),
        ports: config.wires.clone(),
    };
    Box::new(node)
}

fn unix_now() -> crate::Result<i64> {
    let now = SystemTime::now();

    // 获取UNIX Epoch
    let epoch = UNIX_EPOCH;

    // 计算时间间隔
    let duration = now.duration_since(epoch)?;

    // 获取毫秒数
    Ok(duration.as_millis() as i64)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node))
}
