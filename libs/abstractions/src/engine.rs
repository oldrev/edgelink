use std::{sync::Arc, cell::RefCell};
use tokio::sync::{Mutex};
use async_trait::async_trait;
use crate::nodes::*;
use crate::Variant;


#[async_trait]
pub trait FlowBehavior {
    fn id(&self) -> u64;
    fn name(&self) -> &str;
    async fn start(&self);
    async fn stop(&self);
}

#[async_trait]
pub trait FlowEngine {
    async fn start(&self);
    async fn stop(&self);
}

#[derive(Clone)]
pub struct Engine {
    pub nodes: Arc<Mutex<Vec<Box<dyn FlowBehavior>>>>,
}

#[async_trait]
pub trait FlowEngineBehavior {
    async fn start(&self);
    async fn stop(&self);
}
