use std::collections::BTreeMap;
use std::sync::Arc;

use edgelink_abstractions::Result;
use edgelink_abstractions::{nodes::MetaNode, Registry};
use inventory;

use crate::nodes::BuiltinNodeDescriptor;

#[derive(Default)]
pub struct RegistryImpl {
    meta_nodes: Arc<BTreeMap<&'static str, MetaNode>>,
}

impl RegistryImpl {
    pub fn new() -> Result<Self> {
        let mut nodes = BTreeMap::new();
        for bnd in inventory::iter::<BuiltinNodeDescriptor> {
            nodes.insert(bnd.meta.type_name, bnd.meta);
        }
        Ok(RegistryImpl {
            meta_nodes: Arc::new(nodes),
        })
    }
}

impl Registry for RegistryImpl {
    fn all(&self) -> &BTreeMap<&'static str, MetaNode> {
        &self.meta_nodes
    }

    fn get(&self, type_name: &str) -> Option<&MetaNode> {
        self.meta_nodes.get(type_name)
    }
}
