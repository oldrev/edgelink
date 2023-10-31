use std::collections::BTreeMap;

use crate::EdgeLinkError;

#[non_exhaustive]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub enum Variant {
    /// Null
    Null,

    /// An integer
    Integer(i64),

    /// A float
    Float(f64),

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
    pub fn empty_object() -> Variant {
        Variant::Object(BTreeMap::new())
    }

    pub fn is_integer(&self) -> bool {
        matches!(self, Variant::Integer(_))
    }

    pub fn as_integer(&self) -> Option<i64> {
        match self {
            Variant::Integer(int) => Some(*int),
            _ => None,
        }
    }

    pub fn into_integer(self) -> Result<i64, Self> {
        match self {
            Variant::Integer(int) => Ok(int),
            other => Err(other),
        }
    }

    pub fn is_bytes(&self) -> bool {
        self.as_bytes().is_some()
    }

    pub fn as_bytes(&self) -> Option<&Vec<u8>> {
        match *self {
            Variant::Bytes(ref bytes) => Some(bytes),
            _ => None,
        }
    }

    pub fn as_bytes_mut(&mut self) -> Option<&mut Vec<u8>> {
        match *self {
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

    pub fn is_float(&self) -> bool {
        self.as_float().is_some()
    }

    pub fn as_float(&self) -> Option<f64> {
        match *self {
            Variant::Float(f) => Some(f),
            _ => None,
        }
    }

    pub fn into_float(self) -> Result<f64, Self> {
        match self {
            Variant::Float(f) => Ok(f),
            other => Err(other),
        }
    }

    pub fn is_string(&self) -> bool {
        self.as_string().is_some()
    }

    pub fn as_string(&self) -> Option<&str> {
        match *self {
            Variant::String(ref s) => Some(s),
            _ => None,
        }
    }

    pub fn as_string_mut(&mut self) -> Option<&mut String> {
        match *self {
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
        self.as_bool().is_some()
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
        self.as_array().is_some()
    }

    pub fn as_array(&self) -> Option<&Vec<Variant>> {
        match *self {
            Variant::Array(ref array) => Some(array),
            _ => None,
        }
    }

    pub fn as_array_mut(&mut self) -> Option<&mut Vec<Variant>> {
        match *self {
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
        self.as_object().is_some()
    }

    pub fn as_object(&self) -> Option<&BTreeMap<String, Variant>> {
        match *self {
            Variant::Object(ref object) => Some(object),
            _ => None,
        }
    }

    pub fn as_object_mut(&mut self) -> Option<&mut BTreeMap<String, Variant>> {
        match *self {
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
    Integer(i64),
    Integer(u32),
    Integer(i32),
    Integer(u16),
    Integer(i16),
    Integer(u8),
    Integer(i8),

    Bytes(Vec<u8>),

    Float(f64),
    Float(f32),

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
        let mut v = String::with_capacity(1);
        v.push(value);
        Variant::String(v)
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

impl From<&[(&str, Variant)]> for Variant {
    #[inline]
    fn from(value: &[(&str, Variant)]) -> Self {
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
                if number.is_i64() || number.is_u64() {
                    Variant::Integer(number.as_i64().unwrap())
                } else {
                    Variant::Float(number.as_f64().unwrap())
                }
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
                if number.is_i64() || number.is_u64() {
                    Variant::Integer(number.as_i64().unwrap())
                } else {
                    Variant::Float(number.as_f64().unwrap())
                }
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
