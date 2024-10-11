use std::borrow::Cow;

use regex::Regex;
use serde::Deserialize;
use smallvec::SmallVec;

use crate::runtime::flow::*;
use crate::runtime::model::*;
use crate::runtime::nodes::*;
use crate::utils;
use crate::*;

/// Get value of environment variable.
fn evaluate_env_property(name: &str, node: Option<&dyn FlowNodeBehavior>, flow: Option<&Flow>) -> Option<Variant> {
    if let Some(node) = node {
        if let Some(var) = node.get_env(name) {
            return Some(var);
        }
    }

    if let Some(flow_ref) = flow {
        if let Some(node) = node {
            if let Some(ref group) = node.group() {
                return group.get_env(name);
            }
        }

        return flow_ref.get_env(name);
    }

    flow.and_then(|f| f.engine()).or(node.and_then(|n| n.engine())).and_then(|x| x.get_env(name))
}

/// Evaluates a property value according to its type.
///
/// # Arguments
///
/// * `value`       - the raw value
///
/// # Returns
/// The evaluated result
pub async fn evaluate_node_property(
    value: &str,
    _type: RedPropertyType,
    node: Option<&dyn FlowNodeBehavior>,
    flow: Option<&Flow>,
    msg: Option<&Msg>,
) -> crate::Result<Variant> {
    match _type {
        RedPropertyType::Str => Ok(Variant::String(value.into())),

        RedPropertyType::Num | RedPropertyType::Json => {
            let jv: serde_json::Value = serde_json::from_str(value)?;
            Ok(Variant::deserialize(jv)?)
        }

        RedPropertyType::Re => Ok(Variant::Regexp(Regex::new(value)?)),

        RedPropertyType::Date => match value {
            "object" => Ok(Variant::now()),
            "iso" => Ok(Variant::String(utils::time::iso_now())),
            _ => Ok(Variant::Number(utils::time::unix_now().into())),
        },

        RedPropertyType::Bin => {
            let jv: serde_json::Value = serde_json::from_str(value)?;
            let arr = Variant::deserialize(&jv)?;
            let bytes = arr
                .to_bytes()
                .ok_or(EdgelinkError::BadArgument("value"))
                .with_context(|| format!("Expected an array of bytes, got: {:?}", value))?;
            Ok(Variant::from(bytes))
        }

        RedPropertyType::Msg => {
            if let Some(msg) = msg {
                if let Some(pv) = msg.get_nav_stripped(value) {
                    Ok(pv.clone())
                } else {
                    Err(EdgelinkError::BadArgument("value"))
                        .with_context(|| format!("Cannot get the property(s) from `msg`: {}", value))
                }
            } else {
                Err(EdgelinkError::BadArgument("msg")).with_context(|| ("`msg` is not existed!".to_string()))
            }
        }

        RedPropertyType::Global => {
            let ctx_prop = crate::runtime::context::evaluate_key(value)?;
            let ctx = flow
                .and_then(|f| f.engine())
                .or(node.and_then(|n| n.engine()))
                .map(|e| e.context().clone())
                .ok_or_else(|| EdgelinkError::BadArgument("flow,node"))?;

            let msg_env = msg.map(|m| SmallVec::from([PropexEnv::ExtRef("msg", m.as_variant())])).unwrap_or_default();
            if let Some(ctx_value) = ctx.get_one(ctx_prop.store, ctx_prop.key, &msg_env).await {
                Ok(ctx_value)
            } else {
                Err(EdgelinkError::BadArgument("value"))
                    .with_context(|| format!("Cannot found the global context variable `{}`", value))
            }
        }

        RedPropertyType::Flow => {
            let ctx_prop = crate::runtime::context::evaluate_key(value)?;
            let ctx = flow
                .cloned()
                .or(node.and_then(|n| n.flow()))
                .map(|e| e.context().clone())
                .ok_or_else(|| EdgelinkError::BadArgument("flow,node"))?;

            let msg_env = msg.map(|m| SmallVec::from([PropexEnv::ExtRef("msg", m.as_variant())])).unwrap_or_default();
            if let Some(ctx_value) = ctx.get_one(ctx_prop.store, ctx_prop.key, &msg_env).await {
                Ok(ctx_value)
            } else {
                Err(EdgelinkError::BadArgument("value"))
                    .with_context(|| format!("Cannot found the flow context variable `{}`", value))
            }
        }

        RedPropertyType::Bool => Ok(Variant::Bool(value.trim_ascii().parse::<bool>()?)),

        RedPropertyType::Jsonata => todo!(),

        RedPropertyType::Env => match evaluate_env_property(value, node, flow) {
            Some(ev) => Ok(ev),
            _ => Err(EdgelinkError::BadArgument("value"))
                .with_context(|| format!("Cannot found the environment variable `{}`", value)),
        },
    }
}

/// Evaluates a property variant according to its type.
pub fn evaluate_node_property_variant<'a>(
    value: &'a Variant,
    type_: &'a RedPropertyType,
    node: Option<&'a dyn FlowNodeBehavior>,
    flow: Option<&'a Flow>,
    msg: Option<&'a Msg>,
) -> crate::Result<Cow<'a, Variant>> {
    let res = match (type_, value) {
        (RedPropertyType::Str, Variant::String(_)) => Cow::Borrowed(value),
        (RedPropertyType::Re, Variant::Regexp(_)) => Cow::Borrowed(value),
        (RedPropertyType::Num, Variant::Number(_)) => Cow::Borrowed(value),
        (RedPropertyType::Bool, Variant::Bool(_)) => Cow::Borrowed(value),
        (RedPropertyType::Bin, Variant::Bytes(_)) => Cow::Borrowed(value),
        (RedPropertyType::Date, Variant::Date(_)) => Cow::Borrowed(value),
        (RedPropertyType::Json, Variant::Object(_) | Variant::Array(_)) => Cow::Borrowed(value),

        (RedPropertyType::Bin, Variant::Array(array)) => Cow::Owned(Variant::bytes_from_vec(array)?),

        (RedPropertyType::Num | RedPropertyType::Json, Variant::String(s)) => {
            let jv: serde_json::Value = serde_json::from_str(s)?;
            Cow::Owned(Variant::deserialize(jv)?)
        }

        (RedPropertyType::Re, Variant::String(re)) => Cow::Owned(Variant::Regexp(Regex::new(re)?)),

        (RedPropertyType::Date, Variant::String(s)) => match s.as_str() {
            "object" => Cow::Owned(Variant::now()),
            "iso" => Cow::Owned(Variant::String(utils::time::iso_now())),
            _ => Cow::Owned(Variant::Number(utils::time::unix_now().into())),
        },

        (RedPropertyType::Bin, Variant::String(s)) => {
            let jv: serde_json::Value = serde_json::from_str(s.as_str())?;
            let arr = Variant::deserialize(&jv)?;
            let bytes = arr
                .to_bytes()
                .ok_or(EdgelinkError::BadArgument("value"))
                .with_context(|| format!("Expected an array of bytes, got: {:?}", value))?;
            Cow::Owned(Variant::from(bytes))
        }

        (RedPropertyType::Msg, Variant::String(prop)) => {
            if let Some(msg) = msg {
                if let Some(pv) = msg.get_nav_stripped(prop.as_str()) {
                    Cow::Owned(pv.clone())
                } else {
                    return Err(EdgelinkError::BadArgument("value"))
                        .with_context(|| format!("Cannot get the property(s) from `msg`: {}", prop.as_str()));
                }
            } else {
                return Err(EdgelinkError::BadArgument("msg")).with_context(|| "`msg` is required".to_string());
            }
        }

        // process the context variables
        (RedPropertyType::Flow | RedPropertyType::Global, _) => todo!(),

        (RedPropertyType::Bool, Variant::String(s)) => Cow::Owned(Variant::Bool(s.trim_ascii().parse::<bool>()?)),

        (RedPropertyType::Jsonata, _) => todo!(),

        (RedPropertyType::Env, Variant::String(s)) => match evaluate_env_property(s, node, flow) {
            Some(ev) => Cow::Owned(ev),
            _ => {
                return Err(EdgelinkError::BadArgument("value"))
                    .with_context(|| format!("Cannot found the environment variable: '{}'", s));
            }
        },

        (_, _) => {
            return Err(EdgelinkError::BadArgument("value")).with_context(|| "cannot parse the expr".to_string());
        }
    };

    Ok(res)
}

#[cfg(test)]
mod tests {
    // use super::*;
}
