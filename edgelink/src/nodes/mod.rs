use async_trait::async_trait;
use std::fmt;
use std::sync::{Arc, Weak};
use tokio::sync::Mutex as TokMutex;
use tokio_util::sync::CancellationToken;

use crate::engine::FlowEngine;
use crate::flow::Flow;
use crate::model::{ElementId, MsgReceiver, Port};
use crate::red::json::{RedFlowNodeConfig, RedGlobalNodeConfig};
use crate::Result;

mod common;

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
    Flow(fn(Arc<Flow>, BaseFlowNode, &RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior>),
}

#[derive(Clone, Copy)]
pub struct MetaNode {
    /// The tag of the element
    pub kind: NodeKind,
    pub type_name: &'static str,
    pub factory: NodeFactory,
}

#[derive(Debug)]
pub struct BaseFlowNode {
    pub id: ElementId,
    pub flow: Weak<Flow>,
    pub name: String,
    pub msg_receiver: MsgReceiverWrapper,
    pub ports: Vec<Port>,
}

#[derive(Debug)]
pub struct MsgReceiverWrapper {
    msgs_rx: TokMutex<MsgReceiver>,
}

impl MsgReceiverWrapper {
    pub fn new(rx: MsgReceiver) -> Self {
        MsgReceiverWrapper {
            msgs_rx: TokMutex::new(rx),
        }
    }
}

impl Drop for MsgReceiverWrapper {
    fn drop(&mut self) {
        println!("------------------------- Droping....");
    }
}

#[async_trait]
pub trait NodeBehavior: Send + Sync {
    fn id(&self) -> ElementId;
    fn name(&self) -> &str;
    async fn start(&self, cancel: CancellationToken) -> Result<()>;
    async fn stop(&self, cancel: CancellationToken) -> Result<()>;
}

#[async_trait]
pub trait FlowNodeBehavior: NodeBehavior {
    fn base(&self) -> &BaseFlowNode;

    async fn process(&self, cancel: CancellationToken);
}

pub(crate) struct BuiltinNodeDescriptor {
    pub(crate) meta: MetaNode,
}

impl BuiltinNodeDescriptor {
    pub(crate) const fn new(kind: NodeKind, type_name: &'static str, factory: NodeFactory) -> Self {
        BuiltinNodeDescriptor {
            meta: MetaNode {
                kind,
                type_name,
                factory,
            },
        }
    }
}

inventory::collect!(BuiltinNodeDescriptor);
