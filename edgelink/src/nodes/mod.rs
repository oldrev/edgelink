use async_trait::async_trait;
use std::cell::{Cell, RefCell};
use std::future::Future;
use std::sync::Arc;
use tokio::sync::{Mutex, MutexGuard};
use tokio::task::yield_now;
use tokio::{spawn, task, time};

use crate::engine::*;
use edgelink_abstractions::nodes::*;

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
