use edgelink::engine::*;
use edgelink::flow::*;
use edgelink::nodes::*;
use edgelink_abstractions::engine::FlowBehavior;

mod data;
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
    let x = &data::SIMPLE_FLOWS_JSON_VALUE
        .as_array()
        .unwrap()
        .get(0)
        .unwrap();
    let flow = Flow::new(x, &data::SIMPLE_FLOWS_JSON_VALUE.as_array().unwrap());

    assert_eq!(flow.unwrap().id(), 123);
}
