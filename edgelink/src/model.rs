
#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Copy, Clone)]
pub struct ElementID(pub(crate) u64);

impl std::fmt::Display for ElementID {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{:016x}", self.0)
    }
}

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Clone)]
pub struct Port {
    pub node_ids: Vec<ElementID>,
}

