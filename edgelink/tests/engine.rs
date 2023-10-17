use edgelink::engine::*;
use edgelink::flow::*;
use edgelink::nodes::*;
use edgelink::red::json::load_flows_json;
use edgelink::registry::RegistryImpl;
use std::sync::Arc;
use tokio_util::sync::CancellationToken;

/*
#[cfg(test)]
struct TestGlobalNode {
    base: BaseNode,
}

#[cfg(test)]
#[async_trait]
impl GlobalNodeBehavior for TestGlobalNode {}

#[async_trait]
impl NodeBehavior for TestGlobalNode {
    async fn start(&self) {}
    async fn stop(&self) {}
}
*/

#[tokio::test]
async fn can_create_flow_manually() {
    // data::
    let reg = Arc::new(RegistryImpl::new());
    let engine = FlowEngine::new(reg, "./tests/data/flows.json")
        .await
        .unwrap();
    let cancel = CancellationToken::new();
    engine.start(cancel).await.unwrap();

    // assert_eq!(engine.id(), 0xdee0d1b0cfd62a6cu64);
    // assert_eq!(flow.label(), "Flow 1");
}
