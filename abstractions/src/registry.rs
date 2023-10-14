use std::collections::HashMap;

use crate::nodes::MetaNode;

pub trait Registry {
    fn all(&self) -> &HashMap<&'static str, MetaNode>;
    fn get(&self, type_name: &str) -> Option<&MetaNode>;
}
