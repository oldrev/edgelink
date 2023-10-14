use std::{collections::HashSet, hash::Hash};

use serde::{de::Error, Deserialize, Deserializer};
use serde_json::{Value as JsonValue, Map as JsonMap};

use crate::{EdgeLinkError, Result};

// type RedNodeID = [char; 16];

#[derive(Debug, serde::Deserialize)]
pub struct FlowConfig {
    pub disabled: bool,

    #[serde(deserialize_with = "from_hex")]
    pub id: u64,

    pub info: String,
    pub label: String,
    #[serde(alias = "type")]
    pub type_name: String,
}

pub struct JsonValues {
    pub flows: Vec<JsonValue>,
    pub global_nodes: Vec<JsonValue>,
    pub flow_nodes: Vec<JsonValue>,
}

pub trait RedNodeJsonObject {
    fn get_flow_node_dependencies(&self) -> HashSet<&str>;
}

impl RedNodeJsonObject for JsonMap<String, JsonValue> {
    fn get_flow_node_dependencies(&self) -> HashSet<&str> {
        match self
            .get("wires")
            .and_then(|wires_value| wires_value.as_array())
        {
            Some(wires) => {
                let dependencies: HashSet<&str> = wires
                    .iter()
                    .filter_map(|port| port.as_array())
                    .flatten()
                    .filter_map(|id| id.as_str())
                    .collect();
                dependencies
            }
            None => HashSet::new(),
        }
    }
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
