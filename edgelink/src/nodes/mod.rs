use async_trait::async_trait;
use std::cell::{Cell, RefCell};
use std::fmt;
use std::future::Future;
use std::sync::Arc;
use tokio::task::yield_now;
use tokio::{spawn, task, time};

use crate::engine::FlowEngine;
use crate::flow::Flow;
use crate::red::json::{RedFlowNodeConfig, RedGlobalNodeConfig};
use crate::{EdgeLinkError, Result};

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

pub struct BaseNode {
    pub id: u64,
    pub name: String,
    //pub descriptor: &'static MetaNode,
}

#[async_trait]
pub trait NodeBehavior: Send + Sync {
    async fn start(&self) -> Result<()>;
    async fn stop(&self) -> Result<()>;
}

#[async_trait]
pub trait FlowNodeBehavior: NodeBehavior + Send + Sync {}

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

mod debug_node;
mod inject_node;
