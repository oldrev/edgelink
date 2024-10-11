use std::str::FromStr;
use std::sync::Arc;

use runtime::eval;
use serde::{self, Deserialize};
use serde_with::rust::deserialize_ignore_any;
use tokio::sync::RwLock;

use crate::runtime::flow::Flow;
use crate::runtime::nodes::*;
use edgelink_macro::*;

#[flow_node("switch")]
#[derive(Debug)]
struct SwitchNode {
    base: FlowNode,
    config: SwitchNodeConfig,
    prev_value: RwLock<Variant>,
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
    HasKey,

    #[serde(rename = "jsonata_exp")]
    JsonataExp,

    #[serde(rename = "else")]
    Else,

    #[serde(deserialize_with = "deserialize_ignore_any")]
    Undefined,
}

impl SwitchRuleOperator {
    fn apply(&self, a: &Variant, b: &Variant, c: &Variant, case: bool, parts: &[Variant]) -> crate::Result<bool> {
        match self {
            Self::Equal | Self::Else => Ok(a == b),
            Self::NotEqual => Ok(a != b),
            Self::LessThan => Ok(a < b),
            Self::LessThanEqual => Ok(a <= b),
            Self::GreatThan => Ok(a > b),
            Self::GreatThanEqual => Ok(a >= b),
            Self::Between => Ok((a >= b && a <= c) || (a <= b && a >= c)),
            Self::Regex => match (a, b) {
                (Variant::String(a), Variant::Regexp(b)) => Ok(b.is_match(a)),
                (Variant::String(a), Variant::String(b)) => Ok(regex::Regex::new(b)?.is_match(a)),
                _ => Ok(false),
            },
            Self::IsTrue => Ok(a.as_bool().unwrap_or(false)),
            Self::IsFalse => Ok(a.as_bool().unwrap_or(false)),
            Self::IsNull => Ok(a.is_null()),
            Self::IsNotNull => Ok(!a.is_null()),
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
                Some("boolean") => Ok(a.is_bool()),
                Some("string") => Ok(a.is_string()),
                Some("object") => Ok(a.is_object()),
                _ => Ok(false), // TODO
            },
            Self::HasKey => match (a, b.as_str()) {
                (Variant::Object(a), Some(b)) => Ok(a.contains_key(b)),
                _ => Ok(false),
            },
            _ => Err(EdgelinkError::NotSupported("Unsupported operator".to_owned()).into()),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Deserialize, Default)]
enum SwitchPropertyType {
    #[serde(rename = "msg")]
    #[default]
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

impl SwitchPropertyType {
    fn is_constant(&self) -> bool {
        matches!(self, Self::Str | Self::Num | Self::Jsonata)
    }
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
struct RawSwitchRule {
    #[serde(rename = "t")]
    operator: SwitchRuleOperator,

    #[serde(rename = "v")]
    value: Option<Variant>,

    #[serde(rename = "vt")]
    value_type: Option<SwitchPropertyType>,

    #[serde(default, rename = "v2")]
    value2: Option<Variant>,

    #[serde(default, rename = "v2t")]
    value2_type: Option<SwitchPropertyType>,

    #[serde(default, rename = "case")]
    case: bool,
}

#[derive(Debug, Clone)]
struct SwitchRule {
    operator: SwitchRuleOperator,

    value: RedPropertyValue,

    value_type: SwitchPropertyType,

    value2: Option<RedPropertyValue>,

    value2_type: Option<SwitchPropertyType>,

    case: bool,
}

#[derive(Debug, Clone, Deserialize)]
struct SwitchNodeConfig {
    property: String,

    #[serde(rename = "propertyType", default)]
    property_type: SwitchPropertyType,

    #[serde(rename = "checkall", deserialize_with = "deser_bool_from_string", default = "default_checkall_true")]
    check_all: bool,

    #[serde(default)]
    repair: bool,

    outputs: usize,

    #[serde(skip)]
    rules: Vec<SwitchRule>,
}

impl SwitchNode {
    fn build(_flow: &Flow, base: FlowNode, red_config: &RedFlowNodeConfig) -> crate::Result<Box<dyn FlowNodeBehavior>> {
        let mut node = SwitchNode {
            base,
            config: SwitchNodeConfig::deserialize(&red_config.rest)?,
            prev_value: RwLock::new(Variant::Null),
        };
        let rules = if let Some(rules_json) = red_config.rest.get("rules") {
            Self::evalauate_rules(rules_json)?
        } else {
            Vec::new()
        };
        node.config.rules = rules;
        Ok(Box::new(node))
    }

    fn evalauate_rules(rules_json: &serde_json::Value) -> crate::Result<Vec<SwitchRule>> {
        let raw_rules = Vec::<RawSwitchRule>::deserialize(rules_json)?;
        let mut rules = Vec::with_capacity(raw_rules.len());
        for raw_rule in raw_rules.into_iter() {
            let (vt, v) = match (raw_rule.value_type, raw_rule.value) {
                (None, Some(raw_value)) => {
                    if raw_value.is_number() {
                        (SwitchPropertyType::Num, RedPropertyValue::Constant(raw_value))
                    } else {
                        (SwitchPropertyType::Str, RedPropertyValue::Constant(raw_value))
                    }
                }
                (Some(SwitchPropertyType::Prev), _) => (SwitchPropertyType::Prev, RedPropertyValue::null()),
                (Some(raw_vt), Some(raw_value)) => {
                    if raw_vt.is_constant() {
                        let evaluated = RedPropertyValue::evaluate_constant(&raw_value, raw_vt.try_into()?)?;
                        (raw_vt, evaluated)
                    } else {
                        (raw_vt, RedPropertyValue::Runtime(raw_value.to_string()?))
                    }
                }
                (Some(raw_vt), None) => (raw_vt, RedPropertyValue::null()),
                (None, None) => (SwitchPropertyType::Str, RedPropertyValue::null()),
            };

            let (v2t, v2) = if let Some(raw_v2) = raw_rule.value2 {
                match raw_rule.value2_type {
                    None => {
                        if raw_v2.is_number() {
                            (Some(SwitchPropertyType::Num), Some(RedPropertyValue::Constant(raw_v2)))
                        } else {
                            (Some(SwitchPropertyType::Str), Some(RedPropertyValue::Constant(raw_v2)))
                        }
                    }
                    Some(SwitchPropertyType::Prev) => (Some(SwitchPropertyType::Prev), None),
                    Some(raw_v2t) => {
                        if raw_v2t.is_constant() {
                            let evaluated = RedPropertyValue::evaluate_constant(&raw_v2, raw_v2t.try_into()?)?;
                            (Some(raw_v2t), Some(evaluated))
                        } else {
                            (Some(raw_v2t), Some(RedPropertyValue::Runtime(raw_v2.to_string()?)))
                        }
                    }
                }
            } else {
                (raw_rule.value2_type, None)
            };

            rules.push(SwitchRule {
                operator: raw_rule.operator,
                value: v,
                value_type: vt,
                value2: v2,
                value2_type: v2t,
                case: raw_rule.case,
            });
        }
        Ok(rules)
    }

    async fn dispatch_msg(&self, orig_msg: &MsgHandle, cancel: CancellationToken) -> crate::Result<()> {
        let mut envelopes: SmallVec<[Envelope; 4]> = SmallVec::new();
        {
            let msg = orig_msg.read().await;
            let from_value = self.eval_property_value(&msg).await?;
            for (port, rule) in self.config.rules.iter().enumerate() {
                let v1 = self.get_v1(rule, &msg).await?;
                let v2 = if rule.value2.is_some() { self.get_v2(rule, &msg).await? } else { Variant::Null };
                if rule.operator.apply(&from_value, &v1, &v2, rule.case, &[])? {
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
        match rule.value_type {
            SwitchPropertyType::Prev => Ok(self.prev_value.read().await.clone()),
            _ => {
                eval::evaluate_node_property_value(
                    rule.value.clone(),
                    rule.value_type.try_into().unwrap(),
                    self.flow().as_ref(),
                    Some(self),
                    Some(msg),
                )
                .await
            }
        }
    }

    async fn get_v2(&self, rule: &SwitchRule, msg: &Msg) -> crate::Result<Variant> {
        match (rule.value2_type, &rule.value2) {
            (Some(SwitchPropertyType::Prev), _) => Ok(self.prev_value.read().await.clone()),
            (Some(vt2), Some(v2)) => {
                eval::evaluate_node_property_value(
                    v2.clone(),
                    vt2.try_into().unwrap(),
                    self.flow().as_ref(),
                    Some(self),
                    Some(msg),
                )
                .await
            }
            _ => Err(EdgelinkError::BadArgument("rule").into()),
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
    struct BoolVisitor;

    impl<'de> serde::de::Visitor<'de> for BoolVisitor {
        type Value = bool;

        fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
            formatter.write_str("a boolean string `true` or `false`")
        }

        fn visit_bool<E: serde::de::Error>(self, value: bool) -> Result<Self::Value, E> {
            Ok(value)
        }

        fn visit_str<E: serde::de::Error>(self, value: &str) -> Result<Self::Value, E> {
            match value {
                "true" => Ok(true),
                "false" => Ok(false),
                _ => Err(serde::de::Error::invalid_value(serde::de::Unexpected::Str(value), &self)),
            }
        }
    }

    deserializer.deserialize_any(BoolVisitor)
}

fn default_checkall_true() -> bool {
    true
}
