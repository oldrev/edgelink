use std::borrow::BorrowMut;
use std::collections::BTreeMap;

use crate::variant::Variant;
use std::sync::{Arc, Mutex};

pub struct Msg {
    id: u32,
    data: BTreeMap<String, Variant>,
}

impl Msg {
    pub fn id(&self) ->u32 {
        self.id
    }

    pub fn payload(&self) -> Option<&Variant> {
        self.data.get("payload")
    }

    pub fn payload_mut(&mut self) -> Option<&mut Variant> {
        self.data.borrow_mut().get_mut("payload")
    }
}

impl Clone for Msg {
    fn clone(&self) -> Self {
        Self {
            id: self.id,
            data: self.data.clone(),
        }
    }
}
