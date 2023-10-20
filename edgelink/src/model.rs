use std::sync::{Arc, Weak};

use tokio::sync::mpsc;

use crate::{msg::Msg, nodes::FlowNodeBehavior};

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Copy, Clone)]
pub struct ElementId(pub(crate) u64);

impl std::fmt::Display for ElementId {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{:016x}", self.0)
    }
}

#[derive(Debug, Clone)]
pub struct PortWire {
    pub target_node: Weak<Box<dyn FlowNodeBehavior>>,
    pub msg_sender: tokio::sync::mpsc::Sender<Arc<Msg>>,
}

#[derive(Debug, Clone)]
pub struct Port {
    pub wires: Vec<PortWire>,
}

pub type MsgReceiver = mpsc::Receiver<Arc<Msg>>;
