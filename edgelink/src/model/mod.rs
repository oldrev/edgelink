use std::sync::{Arc, Weak};

use tokio::sync::mpsc;

use crate::{nodes::FlowNodeBehavior};

mod variant;
mod msg;
pub mod propex;

pub use msg::Msg;
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
    pub msg_sender: tokio::sync::mpsc::Sender<Arc<Msg>>,
}

#[derive(Debug)]
pub struct Port {
    pub wires: Vec<PortWire>,
}

pub type MsgSender = mpsc::Sender<Arc<Msg>>;
pub type MsgReceiver = mpsc::Receiver<Arc<Msg>>;
