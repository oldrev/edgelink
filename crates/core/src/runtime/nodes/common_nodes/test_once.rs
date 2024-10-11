use std::sync::Arc;

use crate::runtime::flow::Flow;
use crate::runtime::nodes::*;
use edgelink_macro::*;

#[flow_node("test-once")]
struct TestOnceNode {
    base: FlowNode,
}

impl TestOnceNode {
    fn build(_flow: &Flow, state: FlowNode, _config: &RedFlowNodeConfig) -> crate::Result<Box<dyn FlowNodeBehavior>> {
        let node = TestOnceNode { base: state };
        Ok(Box::new(node))
    }
}

#[async_trait]
impl FlowNodeBehavior for TestOnceNode {
    fn get_node(&self) -> &FlowNode {
        &self.base
    }

    async fn run(self: Arc<Self>, stop_token: CancellationToken) {
        while !stop_token.is_cancelled() {
            let engine = self.engine().expect("The engine cannot be released");

            match self.recv_msg(stop_token.clone()).await {
                Ok(msg) => engine.recv_final_msg(msg).expect("Shoud send final msg to the engine"),
                Err(e) => {
                    match e.downcast_ref::<EdgelinkError>() {
                        Some(EdgelinkError::TaskCancelled) => (),
                        None | Some(_) => eprintln!("Failed to recv_msg(): {:?}", e),
                    }
                    break;
                }
            }
        }
    }
}
