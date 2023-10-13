use crate::nodes::*;
use crate::Variant;
use async_trait::async_trait;
use std::cell::RefCell;
use std::sync::{Arc, Mutex};

#[async_trait]
pub trait FlowBehavior: Send + Sync {
    fn id(&self) -> u64;
    fn label(&self) -> &str;
}

pub type Flows = Arc<Mutex<Vec<Box<dyn FlowBehavior>>>>;

#[async_trait]
pub trait FlowEngineBehavior: Send + Sync {
    // fn get_flows(&self) -> &Flows;
    // fn get_flows_mut(&mut self) -> &mut Flows;
}
