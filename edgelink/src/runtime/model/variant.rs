use std::collections::BTreeMap;
use regex;

use crate::runtime::model::propex;
use crate::EdgeLinkError;

use super::propex::PropexSegment;

#[non_exhaustive]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub enum Variant {
    /// Null
    Null,

    /// A float
    Number(f64),

    /// A string
    String(String),

    /// A boolean
    Bool(bool),

    /// Bytes
    Bytes(Vec<u8>),

    /// An array
    Array(Vec<Variant>),

    /// A object
    Object(BTreeMap<String, Variant>),
}

impl Variant {
    pub fn new_empty_object() -> Variant {
        Variant::Object(BTreeMap::new())
    }

    pub fn is_bytes(&self) -> bool {
        matches!(self, Variant::Bytes(..))
    }

    pub fn as_bytes(&self) -> Option<&Vec<u8>> {
        match self {
            Variant::Bytes(ref bytes) => Some(bytes),
            _ => None,
        }
    }

    pub fn as_bytes_mut(&mut self) -> Option<&mut Vec<u8>> {
        match self {
            Variant::Bytes(ref mut bytes) => Some(bytes),
            _ => None,
        }
    }

    pub fn into_bytes(self) -> Result<Vec<u8>, Self> {
        match self {
            Variant::Bytes(vec) => Ok(vec),
            other => Err(other),
        }
    }

    pub fn is_number(&self) -> bool {
        matches!(self, Variant::Number(..))
    }

    pub fn as_number(&self) -> Option<f64> {
        match *self {
            Variant::Number(f) => Some(f),
            _ => None,
        }
    }

    pub fn into_number(self) -> Result<f64, Self> {
        match self {
            Variant::Number(f) => Ok(f),
            other => Err(other),
        }
    }

    pub fn is_string(&self) -> bool {
        matches!(self, Variant::String(..))
    }

    pub fn as_string(&self) -> Option<&str> {
        match self {
            Variant::String(ref s) => Some(s),
            _ => None,
        }
    }

    pub fn as_string_mut(&mut self) -> Option<&mut String> {
        match self {
            Variant::String(ref mut s) => Some(s),
            _ => None,
        }
    }

    pub fn into_string(self) -> Result<String, Self> {
        match self {
            Variant::String(s) => Ok(s),
            other => Err(other),
        }
    }

    pub fn is_bool(&self) -> bool {
        matches!(self, Variant::Bool(..))
    }

    pub fn as_bool(&self) -> Option<bool> {
        match *self {
            Variant::Bool(b) => Some(b),
            _ => None,
        }
    }

    pub fn into_bool(self) -> Result<bool, Self> {
        match self {
            Variant::Bool(b) => Ok(b),
            other => Err(other),
        }
    }

    pub fn is_null(&self) -> bool {
        matches!(self, Variant::Null)
    }

    pub fn is_array(&self) -> bool {
        matches!(self, Variant::Array(..))
    }

    pub fn as_array(&self) -> Option<&Vec<Variant>> {
        match self {
            Variant::Array(ref array) => Some(array),
            _ => None,
        }
    }

    pub fn as_array_mut(&mut self) -> Option<&mut Vec<Variant>> {
        match self {
            Variant::Array(ref mut list) => Some(list),
            _ => None,
        }
    }

    pub fn into_array(self) -> Result<Vec<Variant>, Self> {
        match self {
            Variant::Array(vec) => Ok(vec),
            other => Err(other),
        }
    }

    pub fn is_object(&self) -> bool {
        matches!(self, Variant::Object(..))
    }

    pub fn as_object(&self) -> Option<&BTreeMap<String, Variant>> {
        match self {
            Variant::Object(ref object) => Some(object),
            _ => None,
        }
    }

    pub fn as_object_mut(&mut self) -> Option<&mut BTreeMap<String, Variant>> {
        match self {
            Variant::Object(ref mut object) => Some(object),
            _ => None,
        }
    }

    pub fn into_object(self) -> Result<BTreeMap<String, Variant>, Self> {
        match self {
            Variant::Object(object) => Ok(object),
            other => Err(other),
        }
    }

    pub fn bytes_from_json_value(jv: &serde_json::Value) -> crate::Result<Variant> {
        match jv {
            serde_json::Value::Array(array) => {
                let mut bytes = Vec::with_capacity(array.len());
                for e in array.iter() {
                    if let Some(byte) = e.as_i64() {
                        if !(0..=0xFF).contains(&byte) {
                            return Err(EdgeLinkError::NotSupported(
                                "Invalid byte value".to_owned(),
                            )
                            .into());
                        }
                        bytes.push(byte as u8)
                    } else {
                        return Err(EdgeLinkError::NotSupported(
                            "Invalid byte JSON value type".to_owned(),
                        )
                        .into());
                    }
                }
                Ok(Variant::Bytes(bytes))
            }
            serde_json::Value::String(string) => Ok(Variant::from(string.as_bytes())),
            _ => Err(EdgeLinkError::NotSupported("Invalid byte JSON Value".to_owned()).into()),
        }
    }

    pub fn get_item_by_propex_segment(&self, pseg: &PropexSegment) -> Option<&Variant> {
        match pseg {
            PropexSegment::IntegerIndex(index) => self.get_array_item(*index),
            PropexSegment::StringIndex(prop) => self.get_object_property(prop),
        }
    }

    pub fn get_item_by_propex_segments(&self, psegs: &[PropexSegment]) -> Option<&Variant> {
        if psegs.len() == 1 {
            return self.get_item_by_propex_segment(&psegs[0]);
        } else if psegs.len() > 1 {
            let mut prev = self;
            for pseg in psegs {
                if let Some(cur) = prev.get_item_by_propex_segment(pseg) {
                    prev = cur;
                } else {
                    return None;
                }
            }
            return Some(prev);
        } else {
            return None;
        }
    }

    pub fn get_object_property(&self, prop: &str) -> Option<&Variant> {
        match self.as_object() {
            Some(obj) => obj.get(prop),
            None => None,
        }
    }

    pub fn get_array_item(&self, index: usize) -> Option<&Variant> {
        match self.as_array() {
            Some(arr) => arr.get(index),
            None => None,
        }
    }

    pub fn get_object_nav_prop(&self, expr: &str) -> Option<&Variant> {
        if let Some(prop_segs) = propex::parse(expr).ok() {
            self.get_item_by_propex_segments(&prop_segs)
        } else {
            None
        }
    }
}

macro_rules! implfrom {
    ($($v:ident($t:ty)),+ $(,)?) => {
        $(
            impl From<$t> for Variant {
                #[inline]
                fn from(value: $t) -> Self {
                    Self::$v(value.into())
                }
            }
        )+
    };
}

implfrom! {
    Number(u32),
    Number(i32),
    Number(u16),
    Number(i16),
    Number(u8),
    Number(i8),

    Bytes(Vec<u8>),

    Number(f32),
    Number(f64),

    String(String),
    String(&str),

    Bool(bool),

    Array(&[Variant]),
    Array(Vec<Variant>),

    // Object(&[(String, Variant)]),
    // Object(&[(&str, Variant)]),
    Object(BTreeMap<String, Variant>),
    // Object(&BTreeMap<String, Variant>),
    // Object(BTreeMap<&str, Variant>),
}

impl From<char> for Variant {
    #[inline]
    fn from(value: char) -> Self {
        Variant::String(value.to_string())
    }
}

impl From<&[(String, Variant)]> for Variant {
    #[inline]
    fn from(value: &[(String, Variant)]) -> Self {
        let map: BTreeMap<String, Variant> =
            value.iter().map(|x| (x.0.clone(), x.1.clone())).collect();
        Variant::Object(map)
    }
}

impl<const N: usize> From<[(&str, Variant); N]> for Variant {
    #[inline]
    fn from(value: [(&str, Variant); N]) -> Self {
        let map: BTreeMap<String, Variant> = value
            .iter()
            .map(|x| (x.0.to_string(), x.1.clone()))
            .collect();
        Variant::Object(map)
    }
}

impl From<&[u8]> for Variant {
    fn from(array: &[u8]) -> Self {
        Variant::Bytes(array.to_vec())
    }
}

impl From<serde_json::Value> for Variant {
    fn from(jv: serde_json::Value) -> Self {
        match jv {
            serde_json::Value::Null => Variant::Null,
            serde_json::Value::Bool(boolean) => Variant::from(boolean),
            serde_json::Value::Number(number) => {
                Variant::Number(number.as_f64().unwrap())
            }
            serde_json::Value::String(string) => Variant::String(string.to_owned()),
            serde_json::Value::Array(array) => {
                Variant::Array(array.iter().map(Variant::from).collect())
            }
            serde_json::Value::Object(object) => {
                let new_map: BTreeMap<String, Variant> = object
                    .iter()
                    .map(|(k, v)| (k.to_owned(), Variant::from(v)))
                    .collect();
                Variant::Object(new_map)
            }
        }
    }
}

impl From<&serde_json::Value> for Variant {
    fn from(jv: &serde_json::Value) -> Self {
        match jv {
            serde_json::Value::Null => Variant::Null,
            serde_json::Value::Bool(boolean) => Variant::from(*boolean),
            serde_json::Value::Number(number) => {
                Variant::Number(number.as_f64().unwrap())
            }
            serde_json::Value::String(string) => Variant::String(string.clone()),
            serde_json::Value::Array(array) => {
                Variant::Array(array.iter().map(Variant::from).collect())
            }
            serde_json::Value::Object(object) => {
                let new_map: BTreeMap<String, Variant> = object
                    .iter()
                    .map(|(k, v)| (k.clone(), Variant::from(v)))
                    .collect();
                Variant::Object(new_map)
            }
        }
    }
}

#[test]
fn variant_clone_should_be_ok() {
    let var1 = Variant::Array(vec![
        Variant::Integer(123),
        Variant::Integer(333),
        Variant::Array(vec![Variant::Integer(901), Variant::Integer(902)]),
    ]);
    let mut var2 = var1.clone();

    let inner_array = var2.as_array_mut().unwrap()[2].as_array_mut().unwrap();
    inner_array[0] = Variant::Integer(999);

    let value1 = var1.as_array().unwrap()[2].as_array().unwrap()[0]
        .as_integer()
        .unwrap();
    let value2 = var2.as_array().unwrap()[2].as_array().unwrap()[0]
        .as_integer()
        .unwrap();

    assert_eq!(value1, 901);
    assert_eq!(value2, 999);
    assert_ne!(value1, value2);
}

#[test]
fn variant_propex_readonly_accessing_should_be_ok() {
    /*
    let obj = Variant::Object(vec![
        Variant::Integer(123),
        Variant::Integer(333),
        Variant::Array(vec![Variant::Integer(901), Variant::Integer(902)]),
    ]);


    */
    let obj1 = Variant::from([
        ("value1", Variant::Integer(123)),
        ("value2", Variant::Float(123.0)),
        (
            "value3",
            Variant::from([
                ("aaa", Variant::Integer(333)),
                ("bbb", Variant::Integer(444)),
                ("ccc", Variant::Integer(555)),
                ("ddd", Variant::Integer(999)),
            ]),
        ),
    ]);

    let value1 = obj1
        .get_object_nav_prop("value1")
        .unwrap()
        .as_integer()
        .unwrap();
    assert_eq!(value1, 123);

    let ccc_1 = obj1
        .get_object_nav_prop("value3.ccc")
        .unwrap()
        .as_integer()
        .unwrap();
    assert_eq!(ccc_1, 555);

    let ccc_2 = obj1
        .get_object_nav_prop("['value3'].ccc")
        .unwrap()
        .as_integer()
        .unwrap();
    assert_eq!(ccc_2, 555);

    let ccc_3 = obj1
        .get_object_nav_prop("['value3'][\"ccc\"]")
        .unwrap()
        .as_integer()
        .unwrap();
    assert_eq!(ccc_3, 555);

    let ddd_1 = obj1
        .get_object_nav_prop("value3.ddd")
        .unwrap()
        .as_integer()
        .unwrap();
    assert_eq!(ddd_1, 999);
}
