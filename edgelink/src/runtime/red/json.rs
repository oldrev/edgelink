use std::collections::{BTreeMap, HashSet};
use std::{fs::File, io::Read};

use serde::Deserializer;
use serde::{de::Error, Deserialize};
use serde_json::Map as JsonMap;
use serde_json::Value as JsonValue;
use topo_sort::TopoSort;

use crate::runtime::model::*;
use crate::{EdgeLinkError, Result};

/// Loading 'flows.js'
pub fn load_flows_json(flows_json_path: &str) -> crate::Result<JsonValues> {
    let mut file = File::open(flows_json_path)?;
    let mut contents = String::new();
    file.read_to_string(&mut contents)?;
    let root_jv: JsonValue = serde_json::from_str(&contents)?;
    load_flows_json_value(&root_jv)
}

pub fn load_flows_json_value(root_jv: &JsonValue) -> crate::Result<JsonValues> {
    let all_values = root_jv.as_array().ok_or(EdgeLinkError::BadFlowsJson(
        "The root node must be a Array".to_string(),
    ))?;

    // 初始化 JsonValues 结构
    let mut flows = Vec::new();
    let mut flow_nodes = BTreeMap::new();
    let mut global_nodes = Vec::new();

    let mut topo_sort = TopoSort::new();
    // 遍历 JSON 数组并分类子元素
    for item in all_values.iter() {
        if let Some(obj) = item.as_object() {
            if let (Some(id_str), Some(type_str)) = (
                obj.get("id").and_then(|x| x.as_str()),
                obj.get("type").and_then(|x| x.as_str()),
            ) {
                match type_str {
                    "tab" => flows.push(item.clone()),
                    "comment" => (),
                    _ => match obj.get("z") {
                        Some(_) => {
                            let deps = obj.get_flow_node_dependencies();
                            topo_sort.insert_from_set(id_str, deps);
                            flow_nodes.insert(id_str, item.clone());
                        }
                        None => {
                            let mut global_config: RedGlobalNodeConfig =
                                serde_json::from_value(item.clone())?;
                            global_config.json = obj.clone();
                            global_nodes.push(global_config);
                        }
                    },
                }
            }
        } else {
            return Err(EdgeLinkError::BadFlowsJson("".to_string()).into());
        }
    }

    let mut sorted_flow_nodes = Vec::with_capacity(flow_nodes.len());
    for node_id in &topo_sort {
        // We check for cycle errors before usage
        match node_id {
            Ok((node_id, _)) => sorted_flow_nodes.push(flow_nodes[node_id].clone()),
            Err(_) => {
                return Err(EdgeLinkError::BadFlowsJson("Unexpected cycle!".to_string()).into())
            }
        }
    }

    let mut flow_configs = Vec::with_capacity(flows.len());
    for flow in flows.iter() {
        let mut flow_config: RedFlowConfig = serde_json::from_value(flow.clone())?;
        flow_config.json = flow.as_object().unwrap().clone();
        let mut flow_node_configs = Vec::new();
        for flow_node in sorted_flow_nodes.iter() {
            if flow_node.get("z") == flow.get("id") {
                let mut node_config: RedFlowNodeConfig = serde_json::from_value(flow_node.clone())?;
                node_config.json = flow_node.as_object().unwrap().clone();
                flow_node_configs.push(node_config);
            }
        }
        flow_config.nodes = flow_node_configs;
        flow_configs.push(flow_config);
    }

    Ok(JsonValues {
        flows: flow_configs,
        global_nodes,
    })
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
            Some(wires) => wires
                .iter()
                .filter_map(|port| port.as_array())
                .flatten()
                .filter_map(|id| id.as_str())
                .collect(),
            None => HashSet::new(),
        }
    }
}

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Clone)]
pub struct RedPortConfig {
    pub node_ids: Vec<ElementId>,
}
// type RedNodeID = [char; 16];

#[derive(Debug, serde::Deserialize)]
pub struct RedFlowConfig {
    pub disabled: Option<bool>,

    // #[serde(deserialize_with = "from_hex")]
    pub id: ElementId,

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
    //#[serde(deserialize_with = "from_hex")]
    pub id: ElementId,

    #[serde(alias = "type")]
    pub type_name: String,

    pub name: String,

    //#[serde(deserialize_with = "from_hex")]
    pub z: ElementId,

    pub active: Option<bool>,

    pub disabled: Option<bool>,

    pub wires: Vec<RedPortConfig>,

    #[serde(skip)]
    pub json: serde_json::Map<String, JsonValue>,
}

#[derive(Debug, serde::Deserialize)]
pub struct RedGlobalNodeConfig {
    //#[serde(deserialize_with = "from_hex")]
    pub id: ElementId,

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

impl<'de> Deserialize<'de> for RedPortConfig {
    fn deserialize<D>(deserializer: D) -> std::result::Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let des: Vec<ElementId> = Deserialize::deserialize(deserializer)?;
        Ok(RedPortConfig { node_ids: des })
    }
}

impl<'de> Deserialize<'de> for ElementId {
    fn deserialize<D>(deserializer: D) -> std::result::Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let v: JsonValue = Deserialize::deserialize(deserializer)?;

        match v {
            JsonValue::String(r) => Ok(u64::from_str_radix(r.as_str(), 16)
                .map(ElementId)
                .map_err(D::Error::custom)),
            JsonValue::Number(num) => {
                if let Some(u64v) = num.as_u64() {
                    return Ok(ElementId(u64v));
                } else {
                    Err(num).map_err(D::Error::custom)
                }
            }
            other => Err(other).map_err(D::Error::custom),
        }?
    }
}

#[derive(Debug)]
struct RedPropertyTriple {
    pub p: String,
    pub vt: String,
    pub v: crate::runtime::model::Variant,
}

fn red_property_triple_parse(jv: &serde_json::Value) -> crate::Result<RedPropertyTriple> {
    let triple = jv
        .as_object()
        .ok_or(EdgeLinkError::BadFlowsJson("Bad JSON".to_string()))?;
    let v = triple
        .get("v")
        .ok_or(EdgeLinkError::BadFlowsJson("Bad JSON".to_string()))?
        .as_str()
        .ok_or(EdgeLinkError::BadFlowsJson("Bad JSON".to_string()))?;
    Ok(RedPropertyTriple {
        p: triple
            .get("p")
            .ok_or(EdgeLinkError::BadFlowsJson("Bad JSON".to_string()))?
            .as_str()
            .ok_or(EdgeLinkError::BadFlowsJson("Bad JSON".to_string()))?
            .to_string(),
        vt: triple
            .get("vt")
            .ok_or(EdgeLinkError::BadFlowsJson("Bad JSON".to_string()))?
            .as_str()
            .ok_or(EdgeLinkError::BadFlowsJson("Bad JSON".to_string()))?
            .to_string(),
        v: Variant::String(v.to_string()),
    })
}

impl RedPropertyTriple {

    pub fn collection_from_json_value(
        jv: &serde_json::Value,
    ) -> crate::Result<Vec<RedPropertyTriple>> {
        if let Some(objects) = jv.as_array() {
            let entries: crate::Result<Vec<RedPropertyTriple>> = objects
                .iter()
                .map(|object| red_property_triple_parse(&object))
                .collect();
            entries
        } else {
            Err(EdgeLinkError::BadFlowsJson("Bad JSON".to_string()).into())
        }
    }

}