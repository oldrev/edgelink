use std::str::FromStr;
use std::sync::Arc;

use runtime::eval;
use serde::Deserialize;

use crate::runtime::flow::Flow;
use crate::runtime::nodes::*;
use edgelink_macro::*;

#[flow_node("switch")]
#[derive(Debug)]
struct SwitchNode {
    base: FlowNode,
    config: SwitchNodeConfig,
}

#[derive(Debug, Clone, Copy, Eq, PartialEq, Deserialize)]
enum SwitchRuleOperator {
    #[serde(rename = "eq")]
    Equal,

    #[serde(rename = "neq")]
    NotEqual,

    #[serde(rename = "lt")]
    LessThan,

    #[serde(rename = "lte")]
    LessThanEqual,

    #[serde(rename = "gt")]
    GreatThan,

    #[serde(rename = "gte")]
    GreatThanEqual,

    #[serde(rename = "btwn")]
    Between,

    #[serde(rename = "cont")]
    Contains,

    #[serde(rename = "regex")]
    Regex,

    #[serde(rename = "true")]
    IsTrue,

    #[serde(rename = "false")]
    IsFalse,

    #[serde(rename = "null")]
    IsNull,

    #[serde(rename = "nnull")]
    IsNotNull,

    #[serde(rename = "empty")]
    IsEmpty,

    #[serde(rename = "nempty")]
    IsNotEmpty,

    #[serde(rename = "istype")]
    IsType,

    #[serde(rename = "head")]
    Head,

    #[serde(rename = "tail")]
    Tail,

    #[serde(rename = "index")]
    Index,

    #[serde(rename = "hask")]
    Hask,

    #[serde(rename = "jsonata_exp")]
    JsonataExp,

    #[serde(rename = "else")]
    Else,
}

impl SwitchRuleOperator {
    fn apply(&self, a: &Variant, b: &Variant, c: &Variant, d: &Variant, parts: &[Variant]) -> crate::Result<bool> {
        match self {
            Self::Equal | Self::Else => Ok(a == b),
            Self::NotEqual => Ok(a != b),
            Self::IsTrue => Ok(a.as_bool().unwrap_or(false)),
            Self::IsFalse => Ok(a.as_bool().unwrap_or(false)),
            Self::IsNull => Ok(a.is_null()),
            Self::IsNotNull => Ok(!a.is_null()),
            Self::Between => {
                if let (Some(a), Some(b), Some(c)) = (a.as_i64(), b.as_i64(), c.as_i64()) {
                    Ok((a >= b && a <= c) || (a <= b && a >= c))
                }
                else if let (Some(a), Some(b), Some(c)) = (a.as_f64(), b.as_f64(), c.as_f64()) {
                    Ok((a >= b && a <= c) || (a <= b && a >= c))
                } else {
                    Ok(false)
                }
            }
            Self::IsEmpty => match a {
                Variant::String(a) => Ok(a.is_empty()),
                Variant::Array(a) => Ok(a.is_empty()),
                Variant::Bytes(a) => Ok(a.is_empty()),
                Variant::Object(a) => Ok(a.is_empty()),
                _ => Ok(false),
            },
            Self::IsNotEmpty => match a {
                Variant::String(a) => Ok(!a.is_empty()),
                Variant::Array(a) => Ok(!a.is_empty()),
                Variant::Bytes(a) => Ok(!a.is_empty()),
                Variant::Object(a) => Ok(!a.is_empty()),
                _ => Ok(false),
            },
            Self::Contains => match (a.as_str(), b.as_str()) {
                (Some(a), Some(b)) => Ok(a.contains(b)),
                _ => Ok(false),
            },
            Self::IsType => match b.as_str() {
                Some("array") => Ok(a.is_array()),
                Some("buffer") => Ok(a.is_bytes()),
                Some("json") => {
                    if let Ok(_) = serde_json::Value::from_str(a.as_str().unwrap_or_default()) {
                        Ok(true)
                    } else {
                        Ok(false)
                    }
                }
                Some("null") => Ok(a.is_null()),
                Some("number") => Ok(a.is_number()),
                _ => Ok(false), // TODO
            },
            _ => Err(EdgelinkError::NotSupported("Unsupported operator".to_owned()).into()),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Deserialize)]
enum SwitchPropertyType {
    #[serde(rename = "msg")]
    Msg,

    #[serde(rename = "flow")]
    Flow,

    #[serde(rename = "global")]
    Global,

    #[serde(rename = "str")]
    Str,

    #[serde(rename = "num")]
    Num,

    #[serde(rename = "jsonata")]
    Jsonata,

    #[serde(rename = "env")]
    Env,

    #[serde(rename = "prev")]
    Prev,
}

impl TryFrom<SwitchPropertyType> for RedPropertyType {
    type Error = EdgelinkError;

    fn try_from(value: SwitchPropertyType) -> Result<Self, Self::Error> {
        match value {
            SwitchPropertyType::Msg => Ok(RedPropertyType::Msg),
            SwitchPropertyType::Flow => Ok(RedPropertyType::Flow),
            SwitchPropertyType::Global => Ok(RedPropertyType::Global),
            SwitchPropertyType::Str => Ok(RedPropertyType::Str),
            SwitchPropertyType::Num => Ok(RedPropertyType::Num),
            SwitchPropertyType::Jsonata => Ok(RedPropertyType::Jsonata),
            SwitchPropertyType::Env => Ok(RedPropertyType::Env),
            SwitchPropertyType::Prev => Err(EdgelinkError::BadArgument("self").into()),
        }
    }
}

#[derive(Debug, Clone, Deserialize)]
struct SwitchRule {
    #[serde(rename = "t")]
    operator: SwitchRuleOperator,

    #[serde(rename = "v")]
    value: String,

    #[serde(rename = "vt")]
    value_type: RedPropertyType,

    #[serde(default, rename = "v2")]
    value2: Option<String>,

    #[serde(default, rename = "v2t")]
    value2_type: Option<RedPropertyType>,
}

#[derive(Debug, Clone, Deserialize)]
struct SwitchNodeConfig {
    property: String,

    #[serde(rename = "propertyType")]
    property_type: SwitchPropertyType,

    #[serde(rename = "checkall", deserialize_with = "deser_bool_from_string")]
    check_all: bool,

    repair: bool,

    outputs: usize,

    rules: Vec<SwitchRule>,
}

impl SwitchNode {
    fn build(_flow: &Flow, base: FlowNode, red_config: &RedFlowNodeConfig) -> crate::Result<Box<dyn FlowNodeBehavior>> {
        let node = SwitchNode { base, config: SwitchNodeConfig::deserialize(&red_config.rest)? };
        Ok(Box::new(node))
    }

    async fn dispatch_msg(&self, orig_msg: &MsgHandle, cancel: CancellationToken) -> crate::Result<()> {
        let mut envelopes: SmallVec<[Envelope; 4]> = SmallVec::new();
        {
            let msg = orig_msg.read().await;
            let from_value = self.eval_property_value(&msg).await?;
            for (port, rule) in self.config.rules.iter().enumerate() {
                let v1 = self.get_v1(rule, &msg).await?;
                let v2 = if rule.value2.is_some() { self.get_v2(rule, &msg).await? } else { Variant::Null };
                if rule.operator.apply(&from_value, &v1, &v2, &Variant::Null, &[])? {
                    envelopes.push(Envelope { port, msg: orig_msg.clone() });
                    if !self.config.check_all {
                        break;
                    }
                }
            }
        }
        if envelopes.len() > 0 {
            self.fan_out_many(envelopes, cancel).await?;
        }
        Ok(())
    }

    async fn eval_property_value(&self, msg: &Msg) -> crate::Result<Variant> {
        eval::evaluate_raw_node_property(
            &self.config.property,
            self.config.property_type.try_into()?,
            Some(self),
            self.flow().as_ref(),
            Some(msg),
        )
        .await
    }

    async fn get_v1(&self, rule: &SwitchRule, msg: &Msg) -> crate::Result<Variant> {
        eval::evaluate_raw_node_property(&rule.value, rule.value_type, Some(self), None, Some(msg)).await
    }

    async fn get_v2(&self, rule: &SwitchRule, msg: &Msg) -> crate::Result<Variant> {
        if let (Some(vt2), Some(ref v2)) = (rule.value2_type, &rule.value2) {
            eval::evaluate_raw_node_property(v2, vt2, Some(self), None, Some(msg)).await
        } else {
            Err(EdgelinkError::BadArgument("rule").into())
        }
    }
}

#[async_trait]
impl FlowNodeBehavior for SwitchNode {
    fn get_node(&self) -> &FlowNode {
        &self.base
    }

    async fn run(self: Arc<Self>, stop_token: CancellationToken) {
        while !stop_token.is_cancelled() {
            let cancel = stop_token.clone();
            with_uow(self.as_ref(), cancel.clone(), |node, msg| async move {
                // Do the message dispatching
                node.dispatch_msg(&msg, cancel).await
            })
            .await;
        }
    }
}

fn deser_bool_from_string<'de, D>(deserializer: D) -> Result<bool, D::Error>
where
    D: serde::Deserializer<'de>,
{
    let s: &'de str = serde::Deserialize::deserialize(deserializer)?;
    match s {
        "true" => Ok(true),
        "false" => Ok(false),
        _ => Err(serde::de::Error::custom("expected a boolean string `true` or `false`")),
    }
}
