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

    let jv = data::new_simple_flows_json_value();
    // data::
    let x = &jv
        .as_array()
        .unwrap()
        .get(0)
        .unwrap();
    let flow = Flow::new(x, &jv.as_array().unwrap()).unwrap();

    assert_eq!(flow.id(), 0xdee0d1b0cfd62a6cu64);
    assert_eq!(flow.label(), "Flow 1");

}
