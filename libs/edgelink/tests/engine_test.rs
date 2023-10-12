use edgelink::engine::*;
use edgelink::nodes::*;

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

#[test]
fn can_create_flow_manually() {
    let flow = Flow::new(123, "test flow".to_string());
    assert_eq!(flow.id, 123);
}
