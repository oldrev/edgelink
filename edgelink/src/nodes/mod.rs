use async_trait::async_trait;
use std::fmt;
use std::sync::{Arc, Weak};
use tokio_util::sync::CancellationToken;

use crate::engine::FlowEngine;
use crate::flow::Flow;
use crate::model::{ElementID, Port};
use crate::msg::Msg;
use crate::red::json::{RedFlowNodeConfig, RedGlobalNodeConfig};
use crate::Result;

mod debug_node;
mod inject_node;
mod junction_node;

#[derive(Debug, Clone, Copy)]
pub enum NodeState {
    Starting = 0,
    Idle,
    Busy,
    Stopping,
    Stopped,
}

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

#[derive(Clone, Copy)]
pub enum NodeFactory {
    Global(fn(Arc<FlowEngine>, &RedGlobalNodeConfig) -> Box<dyn NodeBehavior>),
    Flow(fn(Arc<Flow>, &RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior>),
}

#[derive(Clone, Copy)]
pub struct MetaNode {
    /// The tag of the element
    pub kind: NodeKind,
    pub type_name: &'static str,
    pub factory: NodeFactory,
}

pub struct FlowNodeInfo {
    pub id: ElementID,
    pub flow: Weak<Flow>,
    pub name: String,
    pub ports: Vec<Port>,
}

#[async_trait]
pub trait NodeBehavior: Send + Sync {
    fn id(&self) -> ElementID;
    fn name(&self) -> &str;
    async fn start(&self, cancel: CancellationToken) -> Result<()>;
    async fn stop(&self, cancel: CancellationToken) -> Result<()>;
}

#[async_trait]
pub trait FlowNodeBehavior: NodeBehavior + Send + Sync {
    fn ports(&self) -> &Vec<Port>;
    async fn fan_in(&self, msg: Arc<Msg>, cancel: CancellationToken) -> crate::Result<()>;
}

pub(crate) struct BuiltinNodeDescriptor {
    pub(crate) meta: MetaNode,
}

impl BuiltinNodeDescriptor {
    pub(crate) const fn new(kind: NodeKind, type_name: &'static str, factory: NodeFactory) -> Self {
        BuiltinNodeDescriptor {
            meta: MetaNode {
                kind: kind,
                type_name: type_name,
                factory: factory,
            },
        }
    }
}

inventory::collect!(BuiltinNodeDescriptor);
