use crate::flow::Flow;
use crate::nodes::*;
use crate::{nodes::*, red::json::RedFlowNodeConfig, Result};
use std::sync::{Arc, Weak};

struct JunctionNode {
    info: FlowNodeInfo,
}

#[async_trait]
impl NodeBehavior for JunctionNode {
    fn id(&self) -> ElementId {
        self.info.id
    }

    fn name(&self) -> &str {
        &self.info.name
    }

    async fn start(&self, cancel: CancellationToken) -> Result<()> {
        Ok(())
    }

    async fn stop(&self, cancel: CancellationToken) -> Result<()> {
        Ok(())
    }
}

#[async_trait]
impl FlowNodeBehavior for JunctionNode {
    fn ports(&self) -> &Vec<Port> {
        &self.info.ports
    }

    async fn fan_in(&self, msg: Arc<Msg>, cancel: CancellationToken) -> crate::Result<()> {
        let flow_ptr = Weak::upgrade(&self.info.flow).unwrap();
        flow_ptr.fan_out_all(msg, cancel.clone()).await
    }
}

fn new_node(flow: Arc<Flow>, config: &RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior> {
    let node = JunctionNode {
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
    BuiltinNodeDescriptor::new(NodeKind::Flow, "junction", NodeFactory::Flow(new_node))
}
