use std::fmt;
use async_trait::async_trait;
use tokio::sync::Mutex;
use std::sync::Arc;

use crate::{
    Result,
    engine::FlowBehavior,
    red::{RedFlowNodeConfig, RedGlobalNodeConfig},
};

#[derive(Debug, Clone, Copy)]
pub enum NodeKind {
    Flow = 0,
    Global = 1,
}

impl fmt::Display for NodeKind {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            NodeKind::Flow => write!(f, "GlobalNode"),
            NodeKind::Global => write!(f, "FlwoNode"),
        }
    }
}

#[derive(Clone, Copy)]
pub enum NodeFactory {
    Global(fn(&RedGlobalNodeConfig) -> Box<dyn NodeBehavior>),
    Flow(fn(&RedFlowNodeConfig) -> Box<dyn FlowNodeBehavior>),
}

#[derive(Clone, Copy)]
pub struct MetaNode {
    /// The tag of the element
    pub kind: NodeKind,
    pub type_name: &'static str,
    pub factory: NodeFactory,
}

pub struct BaseNode {
    pub id: u64,
    pub name: String,
    //pub descriptor: &'static MetaNode,
}

#[async_trait]
pub trait NodeBehavior: Send + Sync {
    async fn start(&self) -> Result<()>;
    async fn stop(&self) -> Result<()>;
}

#[async_trait]
pub trait FlowNodeBehavior: NodeBehavior + Send + Sync {
    fn flow(&self) -> &Box<dyn FlowBehavior>;
    fn flow_mut(&self) -> &mut Box<dyn FlowBehavior>;
}

/*
impl BaseNode {
    pub fn from_json_value(
        reg: &dyn Registry,
        value: &serde_json::Value,
        meta: &'static MetaNode
    ) -> Self {
        // TODO FIXME
        let type_name = value["type"].as_str().unwrap();
        let meta = reg.get(&type_name).unwrap();
        BaseNode {
            id: value["id"].as_u64().unwrap(),
            name: value["name"].to_string(),
            descriptor: meta,
        }
    }
}
*/
