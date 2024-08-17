use std::borrow::BorrowMut;
use std::collections::BTreeMap;

use lazy_static::lazy_static;
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::Arc;

use crate::runtime::model::{ElementId, Variant};

use super::propex::{self, PropexSegment};

#[derive(Debug)]
pub struct Msg {
    id: u32,
    birth_place: ElementId,
    data: BTreeMap<String, Variant>,
}

impl Msg {
    pub fn new_default(birth_place: ElementId) -> Arc<Self> {
        let mut msg = Msg {
            id: Msg::generate_id(),
            birth_place,
            data: BTreeMap::new(),
        };
        msg.data.insert("payload".to_string(), Variant::Null);
        Arc::new(msg)
    }

    pub fn new_with_payload(birth_place: ElementId, payload: Variant) -> Arc<Self> {
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

    pub fn payload(&self) -> &Variant {
        self.data.get("payload").unwrap() // TODO FIXME
    }

    pub fn payload_mut(&mut self) -> Option<&mut Variant> {
        self.data.borrow_mut().get_mut("payload")
    }

    pub fn generate_id() -> u32 {
        let msg_generator_clone = MSG_GENERATOR.clone();
        msg_generator_clone.generate_id()
    }

    pub fn get_property(&self, prop: &str) -> Option<&Variant> {
        self.data.get(prop)
    }

    pub fn get_nav_property(&self, expr: &str) -> Option<&Variant> {
        if let Some(segs) = propex::parse(expr).ok() {
            match segs[0] {
                // The first level of the property expression for 'msg' must be a string, which means it must be
                // `msg['aaa']` or `msg.aaa`, and not `msg[12]`
                PropexSegment::StringIndex(first_prop_name) => {
                    if let Some(first_prop) = self.get_property(first_prop_name) {
                        first_prop.get_item_by_propex_segments(&segs[1..])
                    } else {
                        None
                    }
                }
                _ => None,
            }
        } else {
            None
        }
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
