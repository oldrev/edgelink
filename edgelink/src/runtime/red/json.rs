use std::collections::{BTreeMap, HashSet};
use std::{fs::File, io::Read};

use serde::Deserializer;
use serde::{de::Error, Deserialize};
use serde_json::Map as JsonMap;
use serde_json::Value as JsonValue;
use topological_sort::TopologicalSort;

use crate::runtime::model::*;
use crate::EdgeLinkError;

/// Loading 'flows.js'
pub fn load_flows_json(flows_json_path: &str) -> crate::Result<JsonValues> {
    let mut file = File::open(flows_json_path)?;
    let mut contents = String::new();
    file.read_to_string(&mut contents)?;
    let root_jv: JsonValue = serde_json::from_str(&contents)?;
    load_flows_json_value(&root_jv)
}

pub fn load_flows_json_value(root_jv: &JsonValue) -> crate::Result<JsonValues> {
    let all_values = root_jv.as_array().ok_or(EdgeLinkError::BadFlowsJson())?;

    // 初始化 JsonValues 结构
    let mut flows = Vec::new();
    let mut flow_nodes = BTreeMap::new();
    let mut global_nodes = Vec::new();

    let mut topo_sort = TopologicalSort::<&str>::new();
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
                            for &dep in deps.iter() {
                                topo_sort.add_dependency(dep, id_str);
                            }
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
            return Err(EdgeLinkError::BadFlowsJson().into());
        }
    }

    let mut sorted_flow_nodes = Vec::new();
    while let Some(node_id) = topo_sort.pop() {
        // We check for cycle errors before usage
        let node = flow_nodes[node_id].clone();
        log::debug!(
            "\t -- node.id={}, node.name={}, node.type={}",
            node_id,
            node.get("name").unwrap().as_str().unwrap(),
            node.get("type").unwrap().as_str().unwrap()
        );
        sorted_flow_nodes.push(node);
    }

    let mut flow_configs = Vec::with_capacity(flows.len());
    for flow in flows.iter() {
        let mut flow_config: RedFlowConfig = serde_json::from_value(flow.clone())?;
        flow_config.json = flow.as_object().unwrap().clone();
        let mut flow_node_configs = Vec::new();
        let flow_id = flow.get("id").unwrap();
        let owned_nodes = sorted_flow_nodes
            .iter()
            .filter(|x| x.get("z").map_or(false, |z| z == flow_id))
            .into_iter();
        for flow_node in owned_nodes.into_iter() {
            let mut node_config: RedFlowNodeConfig = serde_json::from_value(flow_node.clone())?;
            node_config.json = flow_node.as_object().unwrap().clone();
            flow_node_configs.push(node_config);
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
    #[serde(default)]
    pub disabled: bool,

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

    #[serde(default)]
    pub name: String,

    //#[serde(deserialize_with = "from_hex")]
    pub z: ElementId,

    pub active: Option<bool>,

    #[serde(default)]
    pub disabled: bool,

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

    #[serde(default)]
    pub name: String,

    pub active: Option<bool>,

    #[serde(default)]
    pub disabled: bool,

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

#[derive(Debug, Clone, Copy)]
pub enum RedPropertyType {
    Str,
    Num,
    Json,
    Re,
    Date,
    Bin,
    Msg,
    Flow,
    Global,
    Bool,
    Jsonata,
    Env,
}

impl RedPropertyType {
    pub fn from(ptype: &str) -> crate::Result<RedPropertyType> {
        match ptype {
            "str" => Ok(RedPropertyType::Str),
            "num" => Ok(RedPropertyType::Num),
            "json" => Ok(RedPropertyType::Json),
            "re" => Ok(RedPropertyType::Re),
            "date" => Ok(RedPropertyType::Date),
            "bin" => Ok(RedPropertyType::Bin),
            "msg" => Ok(RedPropertyType::Msg),
            "flow" => Ok(RedPropertyType::Flow),
            "global" => Ok(RedPropertyType::Global),
            "bool" => Ok(RedPropertyType::Bool),
            "jsonata" => Ok(RedPropertyType::Jsonata),
            "env" => Ok(RedPropertyType::Env),
            _ => Err(EdgeLinkError::BadFlowsJson().into()),
        }
    }
}

#[derive(Debug, Clone)]
pub struct RedPropertyTriple {
    pub p: String,
    pub vt: RedPropertyType,
    pub v: String,
}

fn parse_property_triple(jv: &serde_json::Value) -> crate::Result<RedPropertyTriple> {
    Ok(RedPropertyTriple {
        vt: RedPropertyType::from(
            jv.get("vt")
                .and_then(serde_json::Value::as_str)
                .unwrap_or("str"),
        )?,
        p: jv
            .get("p")
            .ok_or(EdgeLinkError::BadFlowsJson())?
            .as_str()
            .ok_or(EdgeLinkError::BadFlowsJson())?
            .to_string(),

        v: match jv.get("v").and_then(serde_json::Value::as_str) {
            Some(s) => s.to_string(),
            None => "".to_string(),
        },
    })
}

impl RedPropertyTriple {
    pub fn collection_from_json_value(
        jv: &serde_json::Value,
    ) -> crate::Result<Vec<RedPropertyTriple>> {
        if let Some(objects) = jv.as_array() {
            let entries: crate::Result<Vec<RedPropertyTriple>> =
                objects.iter().map(parse_property_triple).collect();
            entries
        } else {
            Err(EdgeLinkError::BadFlowsJson().into())
        }
    }
}

#[test]
fn parse_red_property_triple_should_be_ok() {
    let data = r#"[
        {
            "p": "timestamp",
            "v": "",
            "vt": "date"
        }
    ]"#;

    // Parse the string of data into serde_json::Value.
    let v: serde_json::Value = serde_json::from_str(data).unwrap();
    let triples = RedPropertyTriple::collection_from_json_value(&v).unwrap();
    assert_eq!(1, triples.len());
    assert_eq!("timestamp", triples[0].p);
    assert_eq!("date", triples[0].vt);
}
