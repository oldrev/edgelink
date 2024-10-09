use crate::runtime::model::*;
use anyhow::Context as _;
use serde::{self, Deserialize};

#[derive(Debug, Default, Clone, Copy, PartialEq, serde::Deserialize, PartialOrd)]
pub struct RedElementTypeValue<'a> {
    pub red_type: &'a str,
    pub id: Option<ElementId>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum RedPropertyValue {
    Constant(Variant),
    Runtime(String),
}

#[derive(Debug, Default, Clone, Copy, Eq, PartialEq, serde::Deserialize, PartialOrd, Ord)]
pub enum RedPropertyType {
    #[serde(rename = "str")]
    #[default]
    Str,

    #[serde(rename = "num")]
    Num,

    #[serde(rename = "json")]
    Json,

    #[serde(rename = "re")]
    Re,

    #[serde(rename = "date")]
    Date,

    #[serde(rename = "bin")]
    Bin,

    #[serde(rename = "msg")]
    Msg,

    #[serde(rename = "flow")]
    Flow,

    #[serde(rename = "global")]
    Global,

    #[serde(rename = "bool")]
    Bool,

    #[serde(rename = "jsonata")]
    Jsonata,

    #[serde(rename = "env")]
    Env,
}

impl RedPropertyType {
    pub fn is_constant(&self) -> bool {
        matches!(
            self,
            RedPropertyType::Str
                | RedPropertyType::Num
                | RedPropertyType::Json
                | RedPropertyType::Bin
                | RedPropertyType::Bool
                | RedPropertyType::Re
        )
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RedPropertyTriple {
    property: String,

    type_: RedPropertyType,

    value: RedPropertyValue,
}

impl RedPropertyValue {
    pub fn null() -> Self {
        Self::Constant(Variant::Null)
    }

    pub fn evaluate_constant(value: &Variant, _type: RedPropertyType) -> crate::Result<Self> {
        match (_type, value) {
            (RedPropertyType::Str, Variant::String(_)) => Ok(Self::Constant(value.clone())),
            (RedPropertyType::Num, Variant::Number(_)) => Ok(Self::Constant(value.clone())),
            (RedPropertyType::Re, Variant::Regexp(_)) => Ok(Self::Constant(value.clone())),
            (RedPropertyType::Bool, Variant::Bool(_)) => Ok(Self::Constant(value.clone())),
            (RedPropertyType::Bin, Variant::Bytes(_)) => Ok(Self::Constant(value.clone())),
            (RedPropertyType::Json, Variant::Object(_) | Variant::Array(_)) => Ok(Self::Constant(value.clone())),
            (_, _) => Self::parse_constant(value.as_str().unwrap_or(""), _type),
        }
    }

    fn parse_constant(value: &str, _type: RedPropertyType) -> crate::Result<Self> {
        let res = match _type {
            RedPropertyType::Str => Variant::String(value.into()),

            RedPropertyType::Num | RedPropertyType::Json => {
                let jv: serde_json::Value = serde_json::from_str(value)?;
                Variant::deserialize(jv)?
            }

            RedPropertyType::Re => Variant::Regexp(regex::Regex::new(value)?),

            RedPropertyType::Bin => {
                let jv: serde_json::Value = serde_json::from_str(value)?;
                let arr = Variant::deserialize(&jv)?;
                let bytes = arr
                    .to_bytes()
                    .ok_or(EdgelinkError::BadArgument("value"))
                    .with_context(|| format!("Expected an array of bytes, got: {:?}", value))?;
                Variant::from(bytes)
            }

            RedPropertyType::Bool => Variant::Bool(value.trim_ascii().parse::<bool>()?),

            RedPropertyType::Jsonata => todo!(),

            _ => {
                return Err(EdgelinkError::BadArgument("_type"))
                    .with_context(|| format!("Unsupported constant type `{:?}`", _type))
            }
        };
        Ok(Self::Constant(res))
    }
}
