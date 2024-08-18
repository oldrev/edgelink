use std::cell::RefCell;
use std::collections::BTreeMap;

use lazy_static::lazy_static;
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::Arc;

use tokio::sync::Mutex;

use crate::runtime::model::{ElementId, Variant};
use crate::runtime::red::json::RedPropertyTriple;

use super::propex::{self, PropexSegment};

pub type Envelope = Arc<Mutex<Msg>>;

pub type MsgBody = BTreeMap<String, Variant>;

#[derive(Debug)]
pub struct Msg {
    pub id: u32,
    pub birth_place: ElementId,
    pub body: BTreeMap<String, Variant>,
}

impl Msg {
    pub fn new_default(birth_place: ElementId) -> Arc<Mutex<Self>> {
        let msg = Msg {
            id: Msg::generate_id(),
            birth_place,
            body: BTreeMap::from([("payload".to_string(), Variant::Null)]),
        };
        Arc::new(Mutex::new(msg))
    }

    pub fn new_with_body(
        birth_place: ElementId,
        body: BTreeMap<String, Variant>,
    ) -> Arc<Mutex<Self>> {
        let msg = Msg {
            id: Msg::generate_id(),
            birth_place,
            body: body,
        };
        Arc::new(Mutex::new(msg))
    }

    pub fn new_with_payload(birth_place: ElementId, payload: Variant) -> Arc<Mutex<Self>> {
        let msg = Msg {
            id: Msg::generate_id(),
            birth_place,
            body: BTreeMap::from([("payload".to_string(), payload)]),
        };
        Arc::new(Mutex::new(msg))
    }

    pub fn generate_id() -> u32 {
        let msg_generator_clone = MSG_GENERATOR.clone();
        msg_generator_clone.generate_id()
    }

    pub fn put_property(&mut self, prop: String, value: Variant) {
        self.body
            .entry(prop)
            .and_modify(|e| *e = value.clone())
            .or_insert(value);
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
                        if segs.len() == 1 {
                            Some(first_prop)
                        } else {
                            first_prop.get_item_by_propex_segments(&segs[1..])
                        }
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
        let trimmed_expr = expr.trim();
        if trimmed_expr.starts_with("msg.") {
            self.get_nav_property(&trimmed_expr[4..])
        } else {
            self.get_nav_property(trimmed_expr)
        }
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
