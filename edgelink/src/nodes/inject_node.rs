use crate::flow::Flow;
use crate::nodes::*;
use crate::{nodes::*, red::json::RedFlowNodeConfig, Result};
use std::sync::{Arc, Weak};
use std::time::{SystemTime, UNIX_EPOCH};
use tokio::sync::Mutex;

struct InjectNode {
    base: BaseNode,
    flow: Weak<Flow>,
}

#[async_trait]
impl NodeBehavior for InjectNode {
    fn id(&self) -> ElementID {
        self.base.id
    }

    async fn start(&self) -> Result<()> {
        // tokio::spawn(async move { });
        tokio::spawn(async move {
            println!("start delay");
            tokio::time::sleep(std::time::Duration::from_secs(2)).await;
            println!("end delay");
        });

        Ok(())
    }

    async fn stop(&self) -> Result<()> {
        Ok(())
    }
}

impl FlowNodeBehavior for InjectNode {}

fn new_node(flow: Arc<Flow>, config: &RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior> {
    let node = InjectNode {
        base: BaseNode {
            id: config.id,
            name: config.name.clone(),
        },
        flow: Arc::downgrade(&flow),
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
