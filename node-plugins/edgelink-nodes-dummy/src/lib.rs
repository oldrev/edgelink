use std::sync::Arc;

use async_trait::*;
use tokio_util::sync::CancellationToken;

use edgelink_core::runtime::context::*;
use edgelink_core::runtime::flow::*;
use edgelink_core::runtime::model::json::*;
use edgelink_core::runtime::model::*;
use edgelink_core::runtime::nodes::*;
use edgelink_core::Result;
use edgelink_macro::*;

#[flow_node("dummy")]
struct DummyNode {
    base: FlowNode,
}

impl DummyNode {
    fn build(_flow: &Flow, state: FlowNode, _config: &RedFlowNodeConfig) -> Result<Box<dyn FlowNodeBehavior>> {
        let node = DummyNode { base: state };
        Ok(Box::new(node))
    }
}

#[async_trait]
impl FlowNodeBehavior for DummyNode {
    fn get_node(&self) -> &FlowNode {
        &self.base
    }

    async fn run(self: Arc<Self>, stop_token: CancellationToken) {
        while !stop_token.is_cancelled() {
            let cancel = stop_token.child_token();
            with_uow(self.as_ref(), cancel.child_token(), |node, msg| async move {
                node.fan_out_one(Envelope { port: 0, msg }, cancel.child_token()).await?;
                Ok(())
            })
            .await;
        }
    }
}

pub fn foo() {}
