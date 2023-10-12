use crate::nodes::*;
use crate::Variant;
use async_trait::async_trait;
use std::cell::RefCell;
use std::sync::{Arc, Mutex};

#[async_trait]
pub trait FlowBehavior : Send + Sync {
    fn id(&self) -> u64;
    fn label(&self) -> &str;

    async fn start(&mut self);
    async fn stop(&mut self);
}

pub type Flows = Vec<Arc<Mutex<Box<dyn FlowBehavior>>>>;

#[async_trait]
pub trait FlowEngine: Send + Sync {
    fn get_flows(&self) -> &Flows;
    fn get_flows_mut(&mut self) -> &mut Flows;

    async fn start(&mut self);
    async fn stop(&mut self);
}
