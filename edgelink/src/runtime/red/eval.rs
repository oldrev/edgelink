use serde_json::Value as JsonValue;
use std::any::Any;
use std::sync::Arc;

use crate::{
    runtime::{
        model::{Msg, Variant},
        nodes::NodeBehavior,
    },
    utils, EdgeLinkError,
};

/**
 * Checks if a String contains any Environment Variable specifiers and returns
 * it with their values substituted in place.
 *
 * For example, if the env var `WHO` is set to `Joe`, the string `Hello ${WHO}!`
 * will return `Hello Joe!`.
 * @param  {String} value - the string to parse
 * @param  {Node} node - the node evaluating the property
 * @return {String} The parsed string
 */
fn evaluate_env_property<'a>(value: &str, node: &dyn NodeBehavior) -> crate::Result<&'a str> {
    /*
    var flow = (node && hasOwnProperty.call(node, "_flow")) ? node._flow : null;
    var result;
    if (/^\${[^}]+}$/.test(value)) {
        // ${ENV_VAR}
        var name = value.substring(2,value.length-1);
        result = getSetting(node, name, flow);
    } else if (!/\${\S+}/.test(value)) {
        // ENV_VAR
        result = getSetting(node, value, flow);
    } else {
        // FOO${ENV_VAR}BAR
        return value.replace(/\${([^}]+)}/g, function(match, name) {
            var val = getSetting(node, name, flow);
            return (val === undefined)?"":val;
        });
    }
    return (result === undefined)?"":result;
    */
    todo!()
}

/**
 * Evaluates a property value according to its type.
 *
 * @param  {String}   value    - the raw value
 * @param  {String}   _type     - the type of the value
 * @param  {Node}     node     - the node evaluating the property
 * @param  {Object}   msg      - the message object to evaluate against
 * @param  {Function} callback - (optional) called when the property is evaluated
 * @return {any} The evaluted property, if no `callback` is provided
 */
pub fn evaluate_node_property(
    value: &JsonValue,
    _type: &str,
    node: &dyn NodeBehavior,
    msg: Arc<Msg>,
) -> crate::Result<Variant> {
    let evaluated = match _type {
        "str" => Variant::String(value.as_str().unwrap().to_string()),

        "num" => Variant::Float(value.as_f64().unwrap()),

        "json" => {
            let root_jv: JsonValue = serde_json::from_str(value.as_str().unwrap())?;
            Variant::from(root_jv)
        }

        "re" => Variant::String(value.as_str().unwrap().to_string()),

        "date" => Variant::Integer(utils::time::unix_now().unwrap()),

        "bin" => {
            let jv: JsonValue = serde_json::from_str(value.as_str().unwrap())?;
            Variant::bytes_from_json_value(&jv)?
        }

        "flow" => {
            /*
            var contextKey = parseContextStore(value);
            if (/\[msg/.test(contextKey.key)) {
                // The key has a nest msg. reference to evaluate first
                contextKey.key = normalisePropertyExpression(contextKey.key, msg, true)
            }
            result = node.context()[type].get(contextKey.key,contextKey.store,callback);
            if (callback) {
                return;
            }
                 */
            todo!()
        }

        "global" => {
            /*
            var contextKey = parseContextStore(value);
            if (/\[msg/.test(contextKey.key)) {
                // The key has a nest msg. reference to evaluate first
                contextKey.key = normalisePropertyExpression(contextKey.key, msg, true)
            }
            result = node.context()[type].get(contextKey.key,contextKey.store,callback);
            if (callback) {
                return;
            }
                 */
            todo!()
        }

        "bool" => Variant::Bool(value.as_bool().unwrap()),

        "jsonata" => {
            return Err(EdgeLinkError::NotSupported(
                "Unsupported node property: JSONATA".to_owned(),
            )
            .into());
        }

        "env" => Variant::String(evaluate_env_property(value.as_str().unwrap(), node)?.to_string()),

        _ => {
            return Err(EdgeLinkError::NotSupported(
                format!("Unsupported node property: {0}", _type).to_owned(),
            )
            .into());
        }
    };

    Ok(evaluated)
}
