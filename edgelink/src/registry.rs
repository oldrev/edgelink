use std::collections::BTreeMap;
use std::sync::Arc;

use crate::{nodes::MetaNode, Result};
use inventory;

use crate::nodes::BuiltinNodeDescriptor;

#[derive(Default)]
pub struct Registry {
    meta_nodes: Arc<BTreeMap<&'static str, MetaNode>>,
}

impl Registry {
    pub fn new() -> Result<Self> {
        let mut nodes = BTreeMap::new();
        for bnd in inventory::iter::<BuiltinNodeDescriptor> {
            nodes.insert(bnd.meta.type_name, bnd.meta);
        }
        Ok(Registry {
            meta_nodes: Arc::new(nodes),
        })
    }

    pub fn all(&self) -> &BTreeMap<&'static str, MetaNode> {
        &self.meta_nodes
    }

    pub fn get(&self, type_name: &str) -> Option<&MetaNode> {
        self.meta_nodes.get(type_name)
    }
}
