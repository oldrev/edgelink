use libloading::Library;
use async_trait::async_trait;
use std::cell::{Cell, RefCell};
use std::future::Future;
use std::sync::Arc;
use tokio::sync::{Mutex, MutexGuard};
use tokio::task::yield_now;
use tokio::{spawn, task, time};

use crate::nodes;

#[derive(Clone)]
struct Flow {
    id: u64,
    name: String,
    nodes: Arc<Mutex<Vec<Box<dyn nodes::FlowNodeBehavior>>>>,
}

impl Flow {
    fn get_id(&self) -> u64 {
        self.id
    }

    fn get_name(&self) -> &str {
        &self.name
    }

    fn get_nodes(&self) -> Arc<Mutex<Vec<Box<dyn nodes::FlowNodeBehavior>>>> {
        self.nodes.clone()
    }

    async fn start(&self) {
        println!("Starting Flow (id={0})...", self.id);
        time::sleep(tokio::time::Duration::from_millis(100)).await;
    }

    async fn stop(&self)
    {
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
