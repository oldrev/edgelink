use async_trait::async_trait;
use log;
use std::cell::{Cell, RefCell};
use std::collections::BTreeMap;
use std::future::Future;
use std::sync::Arc;
use tokio::sync::{Mutex, MutexGuard};
use tokio::task::yield_now;
use tokio::{spawn, task, time};

use ciborium::Value;

use crate::nodes::*;
use edgelink_abstractions::Variant;

pub struct Flow {
    pub id: u64,
    pub name: String,
    pub nodes: Arc<Mutex<Vec<Box<dyn FlowNodeBehavior>>>>,
    pub context: Mutex<RefCell<Variant>>,
}

impl Flow {
    pub fn new(id: u64, name: String) -> Self {
        log::info!("Loading flow (id={0}, name='{1}'):", id, name);

        let mut ctx_map = BTreeMap::new();
        let hex_id = format!("{:016x}", id);
        ctx_map.insert("id".to_string(), Variant::String(hex_id));
        ctx_map.insert("name".to_string(), Variant::String(name.clone()));

        for bnd in inventory::iter::<BuiltinNodeDescriptor> {
            println!("-- kind={}, type-name={}", bnd.kind, bnd.type_name);
        }

        Flow {
            id,
            name,
            nodes: Arc::new(Mutex::new(Vec::new())),
            context: Mutex::new(RefCell::new(Variant::Object(ctx_map))),
        }
    }

    fn nodes(&self) -> Arc<Mutex<Vec<Box<dyn FlowNodeBehavior>>>> {
        self.nodes.clone()
    }

    async fn start(&self) {
        println!("Starting Flow (id={0})...", self.id);
        time::sleep(tokio::time::Duration::from_millis(100)).await;
    }

    async fn stop(&self) {
        time::sleep(tokio::time::Duration::from_millis(100)).await;
        println!("Stopping Flow (id={0})...", self.id);
    }
}

#[derive(Clone)]
pub struct Engine {
    nodes: Arc<Mutex<Vec<Box<Flow>>>>,
}

#[async_trait]
pub trait FlowEngineBehavior {
    async fn start(&self);
    async fn stop(&self);
}
