use std::fmt;

use async_trait::async_trait;

#[derive(Debug, Clone, Copy)]
pub enum NodeKind {
    Flow = 0,
    Global = 1,
}

impl fmt::Display for NodeKind {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            NodeKind::Flow => write!(f, "GlobalNode"),
            NodeKind::Global => write!(f, "FlwoNode"),
        }
    }
}

pub trait MetaNode {
    fn kind(&self) -> NodeKind;
    fn type_name(&self) -> &'static str;
}

pub struct BaseNode {
    pub id: u64,
    pub name: String,
    pub descriptor: &'static dyn MetaNode,
}

#[async_trait]
pub trait NodeBehavior: Send {
    async fn start(&self);
    async fn stop(&self);
}

pub struct FlowNode {
    pub base: BaseNode,
}

#[async_trait]
pub trait FlowNodeBehavior: NodeBehavior {}

pub struct GlobalNode {
    pub base: BaseNode,
}

#[async_trait]
pub trait GlobalNodeBehavior: NodeBehavior {}
