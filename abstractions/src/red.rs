use serde::{de::Error, Deserialize, Deserializer};
use serde_json::Value as JsonValue;

use crate::{EdgeLinkError, Result};

// type RedNodeID = [char; 16];

#[derive(Debug, serde::Deserialize)]
pub struct RedFlowConfig {
    pub disabled: Option<bool>,

    #[serde(deserialize_with = "from_hex")]
    pub id: u64,

    pub info: String,
    pub label: String,

    #[serde(alias = "type")]
    pub type_name: String,

    #[serde(skip)]
    pub json: serde_json::Map<String, JsonValue>,

    #[serde(skip)]
    pub nodes: Vec<RedFlowNodeConfig>,
}

#[derive(Debug, serde::Deserialize)]
pub struct RedFlowNodeConfig {

    #[serde(deserialize_with = "from_hex")]
    pub id: u64,

    #[serde(alias = "type")]
    pub type_name: String,

    pub name: String,

    #[serde(deserialize_with = "from_hex")]
    pub z: u64,

    pub active: Option<bool>,

    pub disabled: Option<bool>,

   #[serde(skip)]
    pub json: serde_json::Map<String, JsonValue>,
}

#[derive(Debug, serde::Deserialize)]
pub struct RedGlobalNodeConfig {

    #[serde(deserialize_with = "from_hex")]
    pub id: u64,

    #[serde(alias = "type")]
    pub type_name: String,

    pub name: String,

    pub active: Option<bool>,

    pub disabled: Option<bool>,

   #[serde(skip)]
    pub json: serde_json::Map<String, JsonValue>,
}




pub struct JsonValues {
    pub flows: Vec<RedFlowConfig>,
    pub global_nodes: Vec<RedGlobalNodeConfig>,
}

fn from_hex<'de, D>(deserializer: D) -> std::result::Result<u64, D::Error>
where
    D: Deserializer<'de>,
{
    let v: JsonValue = Deserialize::deserialize(deserializer)?;

    match v {
        JsonValue::String(r) => Ok(u64::from_str_radix(r.as_str(), 16).map_err(D::Error::custom)),
        JsonValue::Number(num) => {
            if let Some(u64v) = num.as_u64() {
                return Ok(u64v);
            } else {
                Err(num).map_err(D::Error::custom)
            }
        }
        other => Err(other).map_err(D::Error::custom),
    }?
}
