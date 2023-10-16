use crate::flow::Flow;
use crate::model::Port;
use crate::msg::Msg;
use crate::nodes::*;
use crate::red::json::*;
use crate::{Result, EdgeLinkError};
use crate::variant::Variant;
use std::sync::{Arc, Weak};
use tokio::sync::Mutex as TokMutex;

struct InjectNode {
    info: FlowNodeInfo,
}

#[async_trait]
impl NodeBehavior for InjectNode {
    fn id(&self) -> ElementID {
        self.info.id
    }

    fn name(&self) -> &str {
        &self.info.name
    }

    async fn start(&self) -> Result<()> {
        let flow_ptr = Weak::upgrade(&self.info.flow).unwrap();
        let self_id = self.id();
        tokio::spawn(async move {
            loop {
                // TODO FIXME
                tokio::time::sleep(std::time::Duration::from_secs(2)).await;
                let now = crate::utils::time::unix_now().unwrap();
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
        &self.info.ports
    }

    async fn fan_in(&self, msg: Arc<Msg>) -> crate::Result<()> {
        Err(EdgeLinkError::NotSupported("This node is a source node".to_string()).into())
    }
}

fn new_node(flow: Arc<Flow>, config: &RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior> {
    let node = InjectNode {
        info: FlowNodeInfo {
            id: config.id,
            flow: Arc::downgrade(&flow),
            name: config.name.clone(),
            ports: config.wires.clone(),
        },
    };
    Box::new(node)
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "inject", NodeFactory::Flow(new_node))
}
