use async_trait::async_trait;
use tokio::select;
use std::fmt;
use std::sync::{Arc, Weak};
use tokio::sync::Mutex as TokMutex;
use tokio_util::sync::CancellationToken;

use crate::runtime::engine::FlowEngine;
use crate::runtime::flow::Flow;
use crate::runtime::model::{ElementId, Msg, MsgReceiver, MsgSender, Port};
use crate::runtime::red::json::{RedFlowNodeConfig, RedGlobalNodeConfig};
use crate::{EdgeLinkError, Result};

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
    Global(fn(Arc<FlowEngine>, &RedGlobalNodeConfig) -> Arc<dyn NodeBehavior>),
    Flow(fn(Arc<Flow>, Arc<BaseFlowNode>, &RedFlowNodeConfig) -> Arc<dyn FlowNodeBehavior>),
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
    pub msg_tx: MsgSender,
    pub msg_rx: MsgReceiverHolder,
    pub ports: Vec<Port>,
}

impl BaseFlowNode {
    pub(crate) async fn wait_for_msg(&self) -> crate::Result<Arc<Msg>> {
        let rx = &mut self.msg_rx.rx.lock().await;
        match rx.recv().await {
            Some(msg) => Ok(msg),
            None => {
                println!("咋个会收不到");
                Err(EdgeLinkError::TaskCancelled.into())
            }
        }
    }
}

#[derive(Debug)]
pub struct MsgReceiverHolder {
    pub rx: TokMutex<MsgReceiver>,
}

impl MsgReceiverHolder {
    pub fn new(rx: MsgReceiver) -> Self {
        MsgReceiverHolder {
            rx: TokMutex::new(rx),
        }
    }
}

#[async_trait]
pub trait NodeBehavior: Send + Sync {
    fn id(&self) -> ElementId;
    fn name(&self) -> &str;
}

#[async_trait]
pub trait FlowNodeBehavior: NodeBehavior {
    fn base(&self) -> &BaseFlowNode;

    async fn run(&self, stop_token: CancellationToken);

    async fn wait_for_msg(&self, cancel: CancellationToken) -> crate::Result<Arc<Msg>> {
        select! {
            _ = cancel.cancelled() => {
                // The token was cancelled
                Err(EdgeLinkError::TaskCancelled.into())
            }
            result = self.base().wait_for_msg() => {
                result
            }
        }
    }
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
