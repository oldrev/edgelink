use async_trait::async_trait;
use std::cell::{Cell, RefCell};
use std::future::Future;
use std::sync::Arc;
use tokio::sync::{Mutex, MutexGuard};
use tokio::task::yield_now;
use tokio::{spawn, task, time};

use crate::engine::*;
use edgelink_abstractions::nodes::*;

struct DebugNode {
    pub base: FlowNode,
}

#[derive(Debug)]
pub struct BuiltinNodeDescriptor {
    pub kind: NodeKind,
    pub type_name: &'static str,
}

impl BuiltinNodeDescriptor {
    pub const fn new(kind: NodeKind, type_name: &'static str) -> Self {
        BuiltinNodeDescriptor {
            kind: kind,
            type_name: type_name,
        }
    }
}

impl MetaNode for BuiltinNodeDescriptor {
    fn kind(&self) -> NodeKind {
        self.kind
    }
    fn type_name(&self) -> &'static str {
        self.type_name
    }
}

inventory::collect!(BuiltinNodeDescriptor);

mod inject_node;
