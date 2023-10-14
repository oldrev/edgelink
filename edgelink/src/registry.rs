use std::collections::HashMap;

use edgelink_abstractions::Result;
use edgelink_abstractions::{nodes::MetaNode, Registry};
use inventory;

use crate::nodes::BuiltinNodeDescriptor;

#[derive(Default)]
pub struct RegistryImpl {
    meta_nodes: HashMap<&'static str, MetaNode>,
}

impl RegistryImpl {
    pub fn new() -> Result<Self> {
        let mut nodes = HashMap::new();
        for bnd in inventory::iter::<BuiltinNodeDescriptor> {
            nodes.insert(bnd.meta.type_name, bnd.meta);
        }
        Ok(RegistryImpl { meta_nodes: nodes })
    }
}

impl Registry for RegistryImpl {
    fn all(&self) -> &HashMap<&'static str, MetaNode> {
        &self.meta_nodes
    }

    fn get(&self, type_name: &str) -> Option<&MetaNode> {
        self.meta_nodes.get(type_name)
    }
}
