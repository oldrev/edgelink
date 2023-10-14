use std::collections::{BTreeMap, HashSet};
use std::{fs::File, io::Read};

use edgelink_abstractions::red::JsonValues;
use edgelink_abstractions::{EdgeLinkError, Result};
use serde_json::Value as JsonValue;
use serde_json::Map as JsonMap;
use topo_sort::TopoSort;

/// Loading 'flows.js'
pub(crate) fn load_flows_json(flows_json_path: &str) -> Result<JsonValues> {
    let mut file = File::open(flows_json_path)?;
    let mut contents = String::new();
    file.read_to_string(&mut contents)?;
    let root_jv: JsonValue = serde_json::from_str(&contents)?;
    let all_values = root_jv.as_array().ok_or(EdgeLinkError::BadFlowsJson(
        "The root node must be a Array".to_string(),
    ))?;

    // 初始化 JsonValues 结构
    let mut flows = Vec::new();
    let mut flow_nodes = BTreeMap::new();
    let mut global_nodes = Vec::new();

    let mut topo_sort = TopoSort::new();
    // 遍历 JSON 数组并分类子元素
    for item in all_values {
        if let Some(obj) = item.as_object() {
            if let Some(value) = obj.get("type") {
                if let Some(type_str) = value.as_str() {
                    if type_str == "tab" {
                        flows.push(item.clone());
                    } else {
                        if obj.get("z").is_some() {
                            let id = obj["id"].as_str().unwrap(); // FIXME TODO
                            let deps = obj.get_flow_node_dependencies();
                            topo_sort.insert_from_set(id, deps);
                            flow_nodes.insert(id, item.clone());
                        } else {
                            global_nodes.push(item.clone());
                        }
                    }
                }
            }
        }
    }

    let mut sorted_flow_nodes = Vec::with_capacity(flow_nodes.len());
    for node_id in &topo_sort {
        // We check for cycle errors before usage
        match node_id {
            Ok((node_id, _)) => sorted_flow_nodes.push(flow_nodes[node_id].clone()),
            Err(_) => panic!("Unexpected cycle!"),
        }
    }

    Ok(JsonValues {
        flows: flows,
        global_nodes: global_nodes,
        flow_nodes: sorted_flow_nodes,
    })
}
pub(crate) trait RedNodeJsonObject {
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
