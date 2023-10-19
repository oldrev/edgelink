use std::borrow::BorrowMut;
use std::collections::BTreeMap;

use lazy_static::lazy_static;
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::Arc;

use crate::model::ElementId;
use crate::variant::Variant;

#[derive(Debug)]
pub struct Msg {
    id: u32,
    birth_place: ElementId,
    data: BTreeMap<String, Variant>,
}

impl Msg {
    pub fn with_payload(birth_place: ElementId, payload: Variant) -> Arc<Self> {
        let mut msg = Msg {
            id: Msg::generate_id(),
            birth_place,
            data: BTreeMap::new(),
        };
        msg.data.insert("payload".to_string(), payload);
        Arc::new(msg)
    }

    pub fn id(&self) -> u32 {
        self.id
    }

    pub fn birth_place(&self) -> ElementId {
        self.birth_place
    }

    pub fn payload(&self) -> Option<&Variant> {
        self.data.get("payload")
    }

    pub fn payload_mut(&mut self) -> Option<&mut Variant> {
        self.data.borrow_mut().get_mut("payload")
    }

    pub fn generate_id() -> u32 {
        let msg_generator_clone = MSG_GENERATOR.clone();
        msg_generator_clone.generate_id()
    }
}

impl Clone for Msg {
    fn clone(&self) -> Self {
        Self {
            id: self.id,
            birth_place: self.birth_place,
            data: self.data.clone(),
        }
    }
}

struct MsgIDGenerator {
    counter: AtomicU32,
}

impl MsgIDGenerator {
    fn new() -> MsgIDGenerator {
        MsgIDGenerator {
            counter: AtomicU32::new(1),
        }
    }

    fn generate_id(&self) -> u32 {
        self.counter.fetch_add(1, Ordering::SeqCst)
    }
}

lazy_static! {
    static ref MSG_GENERATOR: Arc<MsgIDGenerator> = Arc::new(MsgIDGenerator::new());
}
