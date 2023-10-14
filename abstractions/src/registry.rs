use std::collections::BTreeMap;

use crate::nodes::MetaNode;

pub trait Registry {
    fn all(&self) -> &BTreeMap<&'static str, MetaNode>;
    fn get(&self, type_name: &str) -> Option<&MetaNode>;
}
