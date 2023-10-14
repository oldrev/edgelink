use std::{collections::HashSet, hash::Hash};

use serde::{de::Error, Deserialize, Deserializer};
use serde_json::Value;

use crate::{EdgeLinkError, Result};

// type RedNodeID = [char; 16];

#[derive(Debug, serde::Deserialize)]
pub struct FlowConfig {
    #[serde(deserialize_with = "from_hex")]
    pub id: u64,

    #[serde(alias = "type")]
    pub type_name: String,

    pub label: String,
    pub disabled: bool,
    pub info: String,
}

pub trait RedNodeJsonValue {
    fn get_flow_node_dependencies(&self) -> HashSet<&str>;
}

/*
impl RedNodeJsonValue for serde_json::Value {
    fn get_flow_node_dependencies(&self) -> HashSet<&str> {
        // 我们不检查 JSON，JSON 格式由加载时检查
        let mut deps = HashSet::new();
        if let Some(wires_value) = self.get("wires") {
            if let Some(wires) = wires_value.as_array() {
                for port in wires {
                    if let Some(ids) = port.as_array() {
                        for id in ids {
                            if let Some(id_str) = id.as_str() {
                                deps.insert(id_str);
                            }
                        }
                    }
                }
            }
        }
        deps
    }
}
*/

impl RedNodeJsonValue for Value {
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
    let v: Value = Deserialize::deserialize(deserializer)?;

    match v {
        Value::String(r) => Ok(u64::from_str_radix(r.as_str(), 16).map_err(D::Error::custom)),
        Value::Number(num) => {
            if let Some(u64v) = num.as_u64() {
                return Ok(u64v);
            } else {
                Err(num).map_err(D::Error::custom)
            }
        }
        other => Err(other).map_err(D::Error::custom),
    }?
}
