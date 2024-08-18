use std::sync::{Arc, Weak};

use tokio::sync::mpsc;

use crate::runtime::nodes::FlowNodeBehavior;

mod msg;

pub use msg::*;
pub mod propex;
mod variant;

pub use variant::Variant;

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Copy, Clone)]
pub struct ElementId(pub(crate) u64);

impl std::fmt::Display for ElementId {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{:016x}", self.0)
    }
}

#[derive(Debug)]
pub struct PortWire {
    pub target_node_id: ElementId,
    pub target_node: Weak<dyn FlowNodeBehavior>,
    pub msg_sender: tokio::sync::mpsc::Sender<Envelope>,
}

#[derive(Debug)]
pub struct Port {
    pub wires: Vec<PortWire>,
}

pub type MsgSender = mpsc::Sender<Envelope>;
pub type MsgReceiver = mpsc::Receiver<Envelope>;
