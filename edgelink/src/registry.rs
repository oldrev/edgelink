use std::collections::BTreeMap;
use std::sync::Arc;

use crate::nodes::MetaNode;
use inventory;

use crate::nodes::BuiltinNodeDescriptor;

pub trait Registry: Send + Sync {
    fn all(&self) -> &BTreeMap<&'static str, MetaNode>;
    fn get(&self, type_name: &str) -> Option<&MetaNode>;
}

#[derive(Default)]
pub struct RegistryImpl {
    meta_nodes: Arc<BTreeMap<&'static str, MetaNode>>,
}

impl RegistryImpl {
    pub fn new() -> Self {
        let mut nodes = BTreeMap::new();
        for bnd in inventory::iter::<BuiltinNodeDescriptor> {
            nodes.insert(bnd.meta.type_name, bnd.meta);
        }

        RegistryImpl {
            meta_nodes: Arc::new(nodes),
        }
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
