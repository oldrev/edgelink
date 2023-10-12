use libloading::Library;
use async_trait::async_trait;
use std::cell::{Cell, RefCell};
use std::future::Future;
use std::sync::Arc;
use tokio::sync::{Mutex, MutexGuard};
use tokio::task::yield_now;
use tokio::{spawn, task, time};

#[derive(Debug, Clone)]
pub enum NodeKind {
    Flow = 0,
    Global = 1,
}

#[derive(Debug, Clone)]
pub struct NodeDescriptor  {
    kind: NodeKind,
    node_type: String,
}

#[derive(Debug, Clone)]
pub struct BaseNode {
    id: u64,
    name: String,
    descriptor: &'static NodeDescriptor,
}

#[async_trait]
pub trait NodeBehavior : Send {
    async fn start(&self);
    async fn stop(&self);
}

#[derive(Debug, Clone)]
struct FlowNode {
    base: BaseNode,
}

#[async_trait]
pub trait FlowNodeBehavior : NodeBehavior {
}

#[derive(Debug, Clone)]
struct GlobalNode {
    base: BaseNode,
}

#[async_trait]
pub trait GlobalNodeBehavior : NodeBehavior {
}


#[derive(Debug, Clone)]
struct InjectNode {
    base: FlowNode,
}

#[derive(Debug, Clone)]
struct DebugNode {
    base: FlowNode,
}

struct TestGlobalNode {
    base: BaseNode,
}

#[async_trait]
impl GlobalNodeBehavior for TestGlobalNode {
}

#[async_trait]
impl NodeBehavior for TestGlobalNode {
    async fn start(&self) {}
    async fn stop(&self) {}

}

