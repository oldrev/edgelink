use core::f64;
use std::collections::{HashMap, HashSet};
use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};

use crate::utils::topo::TopologicalSorter;
use serde::de;
use serde::Deserialize;
use serde::Deserializer;
use serde_json::Map as JsonMap;
use serde_json::Value as JsonValue;

use crate::runtime::model::ElementId;
use crate::text::json::{option_value_equals_str, EMPTY_ARRAY};
use crate::EdgelinkError;

use super::*;

pub fn load_flows_json_value(root_jv: JsonValue) -> crate::Result<ResolvedFlows> {
    let mut preprocessed = preprocess_subflows(root_jv)?;
    preprocess_merge_subflow_env(&mut preprocessed)?;
    let all_values = preprocessed
        .as_array()
        .ok_or(EdgelinkError::BadFlowsJson("Cannot convert the value into an array".to_string()))?;

    let mut flows = HashMap::new();
    let mut groups = HashMap::new();
    let mut flow_nodes = HashMap::new();
    let mut global_nodes = Vec::new();

    let mut flow_topo_sort = TopologicalSorter::<ElementId>::new();
    let mut group_topo_sort = TopologicalSorter::<ElementId>::new();
    let mut node_topo_sort = TopologicalSorter::<ElementId>::new();

    for jobject in all_values.iter() {
        if let Some(obj) = jobject.as_object() {
            if let (Some(ele_id), Some(type_value)) = (
                obj.get("id").and_then(parse_red_id_value),
                obj.get("type").and_then(|x| x.as_str()).map(|x| parse_red_type_value(x)),
            ) {
                match type_value.red_type {
                    "tab" => {
                        let deps = obj.get_flow_dependencies(all_values);
                        flow_topo_sort.add_vertex(ele_id);
                        flow_topo_sort.add_deps(ele_id, deps);
                        flows.insert(ele_id, jobject.clone());
                    }

                    "subflow" => {
                        if type_value.id.is_some() {
                            // "subflow:aabbccddee" We got a node that links to the subflow
                            let deps = obj.get_flow_node_dependencies();
                            node_topo_sort.add_vertex(ele_id);
                            node_topo_sort.add_deps(ele_id, deps);
                            flow_nodes.insert(ele_id, jobject.clone());
                        } else {
                            // We got the "subflow" itself
                            let deps = obj.get_subflow_dependencies(all_values);
                            flow_topo_sort.add_vertex(ele_id);
                            flow_topo_sort.add_deps(ele_id, deps);
                            flows.insert(ele_id, jobject.clone());
                        }
                    }

                    "group" => match obj.get("z") {
                        Some(_) => {
                            let g: RedGroupConfig = serde_json::from_value(jobject.clone())?;
                            group_topo_sort.add_vertex(ele_id);
                            if let Some(parent_id) = &g.g {
                                group_topo_sort.add_dep(ele_id, *parent_id);
                            }
                            groups.insert(ele_id, g);
                        }
                        None => {
                            return Err(
                                EdgelinkError::BadFlowsJson("The group must have a 'z' property".to_string()).into()
                            );
                        }
                    },

                    "comment" => (),

                    // Dynamic nodes
                    _ => match obj.get("z") {
                        Some(_) => {
                            let deps = obj.get_flow_node_dependencies();
                            node_topo_sort.add_vertex(ele_id);
                            node_topo_sort.add_deps(ele_id, deps);
                            flow_nodes.insert(ele_id, jobject.clone());
                        }
                        None => {
                            let global_config: RedGlobalNodeConfig = serde_json::from_value(jobject.clone())?;
                            global_nodes.push(global_config);
                        }
                    },
                }
            }
        } else {
            return Err(EdgelinkError::BadFlowsJson("The entry in `flows.json` must be an object".to_string()).into());
        }
    }

    let mut sorted_flows = Vec::new();
    for flow_id in flow_topo_sort.dependency_sort().iter() {
        let flow = flows
            .remove(flow_id)
            .ok_or(EdgelinkError::BadFlowsJson(format!("Cannot find the flow_id('{}') in flows", flow_id)))?;
        sorted_flows.push(flow);
    }

    let mut sorted_flow_groups = Vec::new();
    for group_id in group_topo_sort.dependency_sort().iter() {
        let group = groups
            .remove(group_id)
            .ok_or(EdgelinkError::BadFlowsJson(format!("Cannot find the group_id('{}') in flows", group_id)))?;
        sorted_flow_groups.push(group);
    }

    let mut sorted_flow_nodes = Vec::new();
    for node_id in node_topo_sort.dependency_sort().iter() {
        // We check for cycle errors before usage
        if let Some(node) = flow_nodes.remove(node_id) {
            log::debug!(
                "SORTED_NODES: node.id='{}', node.name='{}', node.type='{}'",
                node_id,
                node.get("name").and_then(|x| x.as_str()).unwrap_or(""),
                node.get("type").and_then(|x| x.as_str()).unwrap_or("")
            );
            sorted_flow_nodes.push(node);
        } else {
            return Err(EdgelinkError::BadFlowsJson(format!("Cannot find the node id '{}'", node_id)).into());
        }
    }

    let mut flow_configs = Vec::with_capacity(flows.len());
    for (flow_ordering, flow) in sorted_flows.into_iter().enumerate() {
        let mut flow_config: RedFlowConfig = serde_json::from_value(flow)?;
        flow_config.ordering = flow_ordering;

        flow_config.subflow_node_id = if flow_config.type_name == "subflow" {
            let key_type = format!("subflow:{}", flow_config.id);
            let node =
                all_values.iter().find(|x| x.get("type").and_then(|y| y.as_str()).is_some_and(|y| y == key_type));
            node.and_then(|x| x.get("id")).and_then(parse_red_id_value)
        } else {
            None
        };

        flow_config.groups = sorted_flow_groups.iter().filter(|x| x.z == flow_config.id).cloned().collect();

        let owned_node_jvs = sorted_flow_nodes
            .iter()
            .filter(|x| x.get("z").and_then(parse_red_id_value).map_or(false, |z| z == flow_config.id));

        for (i, flow_node_jv) in owned_node_jvs.into_iter().enumerate() {
            let mut node_config: RedFlowNodeConfig = serde_json::from_value(flow_node_jv.clone())?;
            node_config.ordering = i;
            flow_config.nodes.push(node_config);
        }

        flow_configs.push(flow_config);
    }

    Ok(ResolvedFlows { flows: flow_configs, global_nodes })
}

fn preprocess_subflows(jv_root: JsonValue) -> crate::Result<JsonValue> {
    let elements = jv_root.as_array().unwrap();
    let mut elements_to_delete = HashSet::new();

    #[derive(Debug)]
    struct SubflowPack<'a> {
        subflow_id: &'a str,
        instance: &'a JsonValue,
        subflow: &'a JsonValue,
        children: Vec<&'a JsonValue>,
    }

    let mut subflow_packs = Vec::new();

    // Find out all of subflow related elements
    for jv in elements.iter() {
        if let Some(("subflow", subflow_id)) = jv.get("type").and_then(|x| x.as_str()).and_then(|x| x.split_once(':')) {
            let subflow = elements
                .iter()
                .find(|x| x.get("id").and_then(|y| y.as_str()).is_some_and(|y| y == subflow_id))
                .ok_or(EdgelinkError::BadFlowsJson(format!(
                    "The cannot found the subflow for subflow instance node(id='{}', type='{}', name='{}')",
                    subflow_id,
                    jv.get("type").and_then(|x| x.as_str()).unwrap_or(""),
                    jv.get("name").and_then(|x| x.as_str()).unwrap_or("")
                )))?;

            // All elements belongs to this flow
            let children = elements
                .iter()
                .filter(|x| x.get("z").and_then(|y| y.as_str()).is_some_and(|y| y == subflow_id))
                .collect();

            let pack = SubflowPack { subflow_id, instance: jv, subflow, children };

            elements_to_delete.insert(pack.instance);
            elements_to_delete.insert(pack.subflow);
            elements_to_delete.extend(pack.children.iter());

            subflow_packs.push(pack);
        }
    }

    let mut new_elements = Vec::new();
    let mut id_map: HashMap<String, String> = HashMap::new();

    for pack in subflow_packs.iter() {
        let subflow_new_id = ElementId::new();

        // "subflow" element
        {
            let mut new_subflow = pack.subflow.clone();
            new_subflow["id"] = JsonValue::String(subflow_new_id.to_string());
            id_map.insert(pack.subflow_id.to_string(), new_subflow["id"].as_str().unwrap().to_string());
            new_elements.push(new_subflow);
        }

        // the fixed subflow instance node
        {
            let mut new_instance = pack.instance.clone();
            new_instance["type"] = JsonValue::String(format!("subflow:{}", subflow_new_id));
            new_elements.push(new_instance);
        }

        // The children elements in the subflow
        for old_child in pack.children.iter() {
            let mut new_child = (*old_child).clone();
            new_child["id"] = generate_new_xored_id_value(subflow_new_id, old_child["id"].as_str().unwrap())?;
            id_map.insert(old_child["id"].as_str().unwrap().to_string(), new_child["id"].as_str().unwrap().to_string());
            new_elements.push(new_child);
        }
    }

    // Remap all known properties of the new elements
    for node in new_elements.iter_mut() {
        let node = node.as_object_mut().unwrap();

        if let Some(JsonValue::String(pvalue)) = node.get_mut("z") {
            if let Some(new_id) = id_map.get(pvalue.as_str()) {
                *pvalue = new_id.to_string();
            }
        }

        if let Some(JsonValue::String(pvalue)) = node.get_mut("g") {
            if let Some(new_id) = id_map.get(pvalue.as_str()) {
                *pvalue = new_id.to_string();
            }
        }

        // Replace the nested flow instance `type` property
        if let Some(JsonValue::String(pvalue)) = node.get_mut("type") {
            if let Some(("subflow", old_id)) = pvalue.split_once(':') {
                if let Some(new_id) = id_map.get(old_id) {
                    *pvalue = format!("subflow:{}", new_id);
                }
            }
        }

        // Node with `wires` property
        if let Some(wires) = node.get_mut("wires").and_then(|x| x.as_array_mut()) {
            for wire in wires {
                let wire = wire.as_array_mut().unwrap();
                for id in wire {
                    if let JsonValue::String(pvalue) = id {
                        if let Some(new_id) = id_map.get(pvalue.as_str()) {
                            *pvalue = new_id.to_string();
                        }
                    }
                }
            }
        }

        // Node with `scope` property
        // TODO CHECK TYPE: complete/catch/status
        if let Some(scope) = node.get_mut("scope").and_then(|x| x.as_array_mut()) {
            for id in scope {
                if let JsonValue::String(pvalue) = id {
                    if let Some(new_id) = id_map.get(pvalue.as_str()) {
                        *pvalue = new_id.to_string();
                    }
                }
            }
        }

        // Node with `links` property
        if let Some(links) = node.get_mut("links").and_then(|x| x.as_array_mut()) {
            for id in links {
                if let JsonValue::String(pvalue) = id {
                    if let Some(new_id) = id_map.get(pvalue.as_str()) {
                        *pvalue = new_id.to_string();
                    }
                }
            }
        }

        // Replace the `in` property
        if let Some(JsonValue::Array(in_props)) = node.get_mut("in") {
            for in_item in in_props.iter_mut() {
                for wires_item in in_item["wires"].as_array_mut().unwrap().iter_mut() {
                    if let Some(JsonValue::String(pvalue)) = wires_item.get_mut("id") {
                        if let Some(new_id) = id_map.get(pvalue.as_str()) {
                            *pvalue = new_id.to_string();
                        }
                    }
                }
            }
        }

        // Replace the `out` property
        if let Some(JsonValue::Array(out_props)) = node.get_mut("out") {
            for out_item in out_props.iter_mut() {
                for wires_item in out_item["wires"].as_array_mut().unwrap().iter_mut() {
                    if let Some(JsonValue::String(pvalue)) = wires_item.get_mut("id") {
                        if let Some(new_id) = id_map.get(pvalue.as_str()) {
                            *pvalue = new_id.to_string();
                        }
                    }
                }
            }
        }
    }

    new_elements.extend(elements.iter().filter(|x| !elements_to_delete.contains(x)).cloned());

    Ok(JsonValue::Array(new_elements))
}

fn generate_new_xored_id_value(subflow_id: ElementId, old_id: &str) -> crate::Result<JsonValue> {
    let old_id =
        parse_red_id_str(old_id).ok_or(EdgelinkError::BadFlowsJson(format!("Cannot parse id: '{}'", old_id)))?;
    Ok(JsonValue::String((subflow_id ^ old_id).to_string()))
}

pub fn parse_red_type_value(t: &str) -> RedTypeValue {
    match t.split_once(':') {
        Some((x, y)) => RedTypeValue { red_type: x, id: parse_red_id_str(y) },
        None => RedTypeValue { red_type: t, id: None },
    }
}

pub fn parse_red_id_str(id_str: &str) -> Option<ElementId> {
    id_str.parse().ok()
}

pub fn parse_red_id_value(id_value: &serde_json::Value) -> Option<ElementId> {
    id_value.as_str().and_then(|s| s.parse().ok())
}

pub trait RedFlowJsonObject {
    fn get_flow_dependencies(&self, elements: &[JsonValue]) -> HashSet<ElementId>;
    fn get_subflow_dependencies(&self, elements: &[JsonValue]) -> HashSet<ElementId>;
}

impl RedFlowJsonObject for JsonMap<String, JsonValue> {
    fn get_flow_dependencies(&self, elements: &[JsonValue]) -> HashSet<ElementId> {
        let this_id = self.get("id");
        let child_nodes = elements.iter().filter(|x| x.get("z") == this_id);

        // `wires`` connects to other flow nodes, and in the Node-RED GUI editor,
        // this situation will not occur. However, in manually written test JSON, it will appear.
        let wires_ids = child_nodes.flat_map(|x| flatten_wires(x)).collect::<HashSet<&JsonValue>>();

        let related_link_in_ids = elements
            .iter()
            .filter_map(|x| {
                if x.get("z") == this_id
                    && (option_value_equals_str(&x.get("type"), "link out")
                        || option_value_equals_str(&x.get("type"), "link call"))
                {
                    x.get("links").and_then(|y| y.as_array())
                } else {
                    None
                }
            })
            .flat_map(|array| array.iter())
            .collect::<HashSet<&JsonValue>>();

        elements
            .iter()
            .filter(|x| {
                if let Some(flow_id) = x.get("id") {
                    wires_ids.contains(flow_id)
                        || (option_value_equals_str(&x.get("type"), "link in") && related_link_in_ids.contains(flow_id))
                } else {
                    false
                }
            })
            .filter(|x| x.get("z") != this_id) // Remove itself!
            .filter_map(|x| x.get("z"))
            .filter_map(parse_red_id_value)
            .collect::<HashSet<ElementId>>()
    }

    fn get_subflow_dependencies(&self, elements: &[JsonValue]) -> HashSet<ElementId> {
        let subflow_id = self.get("id").and_then(|x| x.as_str()).expect("Must have `id`");

        elements
            .iter()
            .filter_map(|x| x.as_object())
            .filter(|o| {
                o.get("type")
                    .and_then(|x| x.as_str())
                    .and_then(|x| x.split_once(':'))
                    .is_some_and(|x| x.0 == "subflow" && x.1 == subflow_id)
            })
            .filter_map(|o| o.get("z"))
            .filter_map(parse_red_id_value)
            .collect::<HashSet<ElementId>>()
    }
}

fn flatten_wires<'a>(json_map: &'a serde_json::Value) -> Vec<&'a serde_json::Value> {
    let wires = json_map.get("wires").and_then(serde_json::Value::as_array).unwrap_or(&EMPTY_ARRAY);

    let flattened_wires: Vec<&'a serde_json::Value> =
        wires.iter().flat_map(|sublist| sublist.as_array().unwrap_or(&EMPTY_ARRAY).iter()).collect();

    flattened_wires
}

pub trait RedFlowNodeJsonObject {
    fn get_flow_node_dependencies(&self) -> HashSet<ElementId>;
}

impl RedFlowNodeJsonObject for JsonMap<String, JsonValue> {
    fn get_flow_node_dependencies(&self) -> HashSet<ElementId> {
        let mut result = HashSet::new();

        // Add wires
        if let Some(wires) = self.get("wires").and_then(|wires_value| wires_value.as_array()) {
            let iter = wires.iter().filter_map(|port| port.as_array()).flatten().filter_map(parse_red_id_value);
            result.extend(iter);
        }

        // Add scope
        if let Some(scopes) = self.get("scope").and_then(|wires_value| wires_value.as_array()) {
            let iter = scopes.iter().filter_map(|port| port.as_array()).flatten().filter_map(parse_red_id_value);
            result.extend(iter);
        }

        // Add links
        if let Some(links) = self.get("links").and_then(|x| x.as_array()) {
            let red_type = self.get("type").and_then(|x| x.as_str());
            if red_type == Some("link out") || red_type == Some("link call") {
                let iter = links.iter().filter_map(parse_red_id_value);
                result.extend(iter);
            }
        }

        result
    }
}

pub fn deser_red_id<'de, D>(deserializer: D) -> Result<ElementId, D::Error>
where
    D: Deserializer<'de>,
{
    let s = String::deserialize(deserializer)?;
    s.parse().map_err(serde::de::Error::custom)
}

pub fn deser_red_optional_id<'de, D>(deserializer: D) -> Result<Option<ElementId>, D::Error>
where
    D: Deserializer<'de>,
{
    let s: Option<String> = Option::deserialize(deserializer)?;
    match s {
        Some(s) => {
            if s.is_empty() {
                Ok(None)
            } else {
                s.parse().map_err(serde::de::Error::custom).map(Some)
            }
        }
        None => Ok(None),
    }
}

pub fn deser_red_id_vec<'de, D>(deserializer: D) -> Result<Vec<ElementId>, D::Error>
where
    D: Deserializer<'de>,
{
    let str_ids: Vec<String> = Vec::deserialize(deserializer)?;
    let mut ids = Vec::with_capacity(str_ids.capacity());
    for str_id in str_ids.iter() {
        ids.push(str_id.parse().map_err(serde::de::Error::custom)?);
    }
    Ok(ids)
}

pub(crate) fn deserialize_wires<'de, D>(deserializer: D) -> Result<Vec<RedPortConfig>, D::Error>
where
    D: Deserializer<'de>,
{
    struct WiresVisitor;

    impl<'de> de::Visitor<'de> for WiresVisitor {
        type Value = Vec<RedPortConfig>;

        fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
            formatter.write_str("a list of list of strings representing hex-encoded u64 values")
        }

        fn visit_seq<A>(self, mut seq: A) -> Result<Self::Value, A::Error>
        where
            A: de::SeqAccess<'de>,
        {
            let mut wires = Vec::new();

            while let Some(inner_seq) = seq.next_element::<Vec<String>>()? {
                let mut node_ids = Vec::new();

                for hex_str in inner_seq {
                    let node_id = parse_red_id_str(&hex_str)
                        .ok_or(EdgelinkError::BadFlowsJson(format!("Bad ID string: '{}'", &hex_str)))
                        .map_err(de::Error::custom)?;
                    node_ids.push(node_id);
                }

                wires.push(RedPortConfig { node_ids });
            }

            Ok(wires)
        }
    }

    deserializer.deserialize_seq(WiresVisitor)
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
            _ => Err(EdgelinkError::BadFlowsJson(format!("Unsupported property type: '{}'", ptype)).into()),
        }
    }
}

pub fn str_to_option_u64<'de, D>(deserializer: D) -> Result<Option<u64>, D::Error>
where
    D: Deserializer<'de>,
{
    let value: Option<String> = Option::deserialize(deserializer)?;
    match value {
        Some(s) => {
            if s.is_empty() {
                Ok(None)
            } else {
                s.parse::<u64>()
                    .map(Some)
                    .map_err(|_| de::Error::invalid_value(de::Unexpected::Str(&s), &"An invalid u64"))
            }
        }
        None => Ok(None),
    }
}

pub fn str_to_option_f64<'de, D>(deserializer: D) -> Result<Option<f64>, D::Error>
where
    D: Deserializer<'de>,
{
    struct F64Visitor;

    impl<'de> de::Visitor<'de> for F64Visitor {
        type Value = Option<f64>;

        fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
            formatter.write_str("a float, a string containing a float, or an empty string")
        }

        fn visit_f64<E>(self, value: f64) -> Result<Option<f64>, E>
        where
            E: de::Error,
        {
            Ok(Some(value))
        }

        fn visit_str<E>(self, value: &str) -> Result<Option<f64>, E>
        where
            E: de::Error,
        {
            if value.trim().is_empty() {
                Ok(None)
            } else {
                value.parse::<f64>().map(Some).map_err(de::Error::custom)
            }
        }

        fn visit_string<E>(self, value: String) -> Result<Option<f64>, E>
        where
            E: de::Error,
        {
            self.visit_str(&value)
        }

        fn visit_u64<E>(self, value: u64) -> Result<Option<f64>, E>
        where
            E: de::Error,
        {
            Ok(Some(value as f64))
        }

        fn visit_i64<E>(self, value: i64) -> Result<Option<f64>, E>
        where
            E: de::Error,
        {
            Ok(Some(value as f64))
        }
    }

    deserializer.deserialize_any(F64Visitor)
}

pub fn deser_f64_or_string_nan<'de, D>(deserializer: D) -> Result<f64, D::Error>
where
    D: Deserializer<'de>,
{
    struct F64Visitor;

    impl<'de> de::Visitor<'de> for F64Visitor {
        type Value = f64;

        fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
            formatter.write_str("a float, a string containing a float, or an empty string")
        }

        fn visit_f64<E>(self, value: f64) -> Result<f64, E>
        where
            E: de::Error,
        {
            Ok(value)
        }

        fn visit_str<E>(self, value: &str) -> Result<f64, E>
        where
            E: de::Error,
        {
            if value.trim().is_empty() {
                Ok(f64::NAN)
            } else {
                value.parse::<f64>().map_err(de::Error::custom)
            }
        }

        fn visit_string<E>(self, value: String) -> Result<f64, E>
        where
            E: de::Error,
        {
            self.visit_str(&value)
        }

        fn visit_u64<E>(self, value: u64) -> Result<f64, E>
        where
            E: de::Error,
        {
            Ok(value as f64)
        }

        fn visit_i64<E>(self, value: i64) -> Result<f64, E>
        where
            E: de::Error,
        {
            Ok(value as f64)
        }
    }

    deserializer.deserialize_any(F64Visitor)
}

pub fn str_to_option_u16<'de, D>(deserializer: D) -> Result<Option<u16>, D::Error>
where
    D: Deserializer<'de>,
{
    let value: Option<String> = Option::deserialize(deserializer)?;
    match value {
        Some(s) => {
            if s.is_empty() {
                Ok(None)
            } else {
                s.parse::<u16>()
                    .map(Some)
                    .map_err(|_| de::Error::invalid_value(de::Unexpected::Str(&s), &"An invalid u16"))
            }
        }
        None => Ok(None),
    }
}

pub fn str_to_ipaddr<'de, D>(deserializer: D) -> Result<Option<IpAddr>, D::Error>
where
    D: Deserializer<'de>,
{
    let value: Option<String> = Option::deserialize(deserializer)?;
    match value {
        Some(s) => {
            if s.is_empty() {
                Ok(None)
            }
            // Try parsing as IPv4
            else if let Ok(ipv4) = s.parse::<Ipv4Addr>() {
                Ok(Some(IpAddr::V4(ipv4)))
            }
            // Try parsing as IPv6
            else if let Ok(ipv6) = s.parse::<Ipv6Addr>() {
                Ok(Some(IpAddr::V6(ipv6)))
            }
            // If neither, return an error
            else {
                Err(de::Error::invalid_value(de::Unexpected::Str(&s), &"a valid IP address"))
            }
        }
        None => Ok(None),
    }
}

fn preprocess_merge_subflow_env(flows: &mut JsonValue) -> crate::Result<()> {
    let elements = flows.as_array_mut().ok_or(EdgelinkError::BadArgument("flows"))?;
    let subflows: HashMap<String, JsonValue> = elements
        .iter()
        .filter(|x| x.get("type").and_then(|y| y.as_str()).map(|y| y == "subflow").unwrap_or(false))
        .filter(|x| x.get("env").is_some())
        .map(|e| (e.get("id").and_then(|x| x.as_str()).unwrap().to_string(), e.get("env").cloned().unwrap()))
        .collect();

    for element in elements.iter_mut() {
        if let Some(("subflow", subflow_id)) =
            element.get("type").and_then(|x| x.as_str()).and_then(|x| x.split_once(':'))
        {
            if let Some(subflow_env) = subflows.get(subflow_id) {
                let instance_env = if let Some(instance_env) = element.get_mut("env") {
                    instance_env
                } else {
                    element["env"] = JsonValue::Array(Vec::new());
                    element.get_mut("env").unwrap()
                };
                merge_env(instance_env, subflow_env)?;
            }
        }
    }
    Ok(())
}

fn merge_env(target_envs: &mut JsonValue, ref_envs: &JsonValue) -> crate::Result<()> {
    let target_vec: &mut Vec<JsonValue> =
        target_envs.as_array_mut().ok_or(EdgelinkError::BadArgument("target_envs"))?;
    let ref_vec: &Vec<JsonValue> = ref_envs.as_array().ok_or(EdgelinkError::BadArgument("ref_envs"))?;

    let target_names: HashSet<String> = target_vec
        .iter()
        .filter_map(|item| item.get("name"))
        .filter_map(|name| name.as_str())
        .map(|name| name.to_string())
        .collect();

    for item in ref_vec.iter() {
        if let Some(name) = item.get("name").and_then(|name| name.as_str()) {
            if !target_names.contains(name) {
                target_vec.push(item.clone());
            }
        }
    }

    Ok(())
}
