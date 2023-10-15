use std::borrow::BorrowMut;
use std::collections::BTreeMap;

use std::sync::{Arc, Mutex};
use crate::variant::Variant;

struct Msg {
    data: BTreeMap<String, Variant>,
}

impl Msg {

    fn id(&self) -> Option<i64> {
        let id = self.data.get("id")?;
        id.as_integer()
    }

    fn payload(&self) -> Option<&Variant> {
        self.data.get("payload")
    }

    fn payload_mut(&mut self) -> Option<&mut Variant> {
        self.data.borrow_mut().get_mut("payload")
    }
}

impl Clone for Msg {

    fn clone(&self) -> Self {
        Self { data: self.data.clone() }
    }
}