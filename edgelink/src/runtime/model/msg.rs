use std::borrow::BorrowMut;
use std::collections::BTreeMap;

use lazy_static::lazy_static;
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::Arc;

use tokio::sync::Mutex as TokMutex;

use crate::runtime::model::{ElementId, Variant};
use crate::runtime::red::json::RedPropertyTriple;

use super::propex::{self, PropexSegment};

#[derive(Debug)]
pub struct Msg {
    id: u32,
    birth_place: ElementId,
    body: BTreeMap<String, Variant>,
}

impl Msg {
    pub fn new_default(birth_place: ElementId) -> Arc<Self> {
        let mut msg = Msg {
            id: Msg::generate_id(),
            birth_place,
            body: BTreeMap::new(),
        };
        msg.body.insert("payload".to_string(), Variant::Null);
        Arc::new(msg)
    }

    pub fn new_with_body(birth_place: ElementId, body: BTreeMap<String, Variant>) -> Arc<Self> {
        let msg = Msg {
            id: Msg::generate_id(),
            birth_place,
            body,
        };
        Arc::new(msg)
    }

    pub fn new_with_payload(birth_place: ElementId, payload: Variant) -> Arc<Self> {
        let mut msg = Msg {
            id: Msg::generate_id(),
            birth_place,
            body: BTreeMap::new(),
        };
        msg.body.insert("payload".to_string(), payload);
        Arc::new(msg)
    }

    pub fn id(&self) -> u32 {
        self.id
    }

    pub fn birth_place(&self) -> ElementId {
        self.birth_place
    }

    pub fn payload(&self) -> &Variant {
        self.body.get("payload").unwrap() // TODO FIXME
    }

    pub fn payload_mut(&mut self) -> Option<&mut Variant> {
        self.body.borrow_mut().get_mut("payload")
    }

    pub fn generate_id() -> u32 {
        let msg_generator_clone = MSG_GENERATOR.clone();
        msg_generator_clone.generate_id()
    }

    pub fn get_property(&self, prop: &str) -> Option<&Variant> {
        self.body.get(prop)
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

    pub fn get_trimmed_nav_property(&self, expr: &str) -> Option<&Variant> {
        if expr.trim().starts_with("msg.") {
            self.get_nav_property(&expr[4..])
        } else {
            self.get_nav_property(expr)
        }
    }

    pub fn put_property(&mut self, expr: &str, value: &Variant) {
        self.body
            .entry(expr.to_string())
            .and_modify(|e| *e = value.clone())
            .or_insert(value.clone());
    }
}

impl Clone for Msg {
    fn clone(&self) -> Self {
        Self {
            id: self.id,
            birth_place: self.birth_place,
            body: self.body.clone(),
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
