use std::sync::Arc;

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
    fn apply(
        &self,
        a: &Variant,
        b: &Variant,
        c: &Variant,
        d: &Variant,
        parts: Option<&[Variant]>,
    ) -> crate::Result<bool> {
        match self {
            Self::Equal => Ok(a == b),
            Self::NotEqual => Ok(a != b),
            Self::IsTrue => Ok(a.as_bool().unwrap_or(false)),
            Self::IsFalse => Ok(a.as_bool().unwrap_or(false)),

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

impl SwitchPropertyType {
    fn to_red_type(&self) -> crate::Result<RedPropertyType> {
        match self {
            Self::Msg => Ok(RedPropertyType::Msg),
            Self::Flow => Ok(RedPropertyType::Flow),
            Self::Global => Ok(RedPropertyType::Global),
            Self::Str => Ok(RedPropertyType::Str),
            Self::Num => Ok(RedPropertyType::Num),
            Self::Jsonata => Ok(RedPropertyType::Jsonata),
            Self::Env => Ok(RedPropertyType::Env),
            Self::Prev => Err(EdgelinkError::BadArgument("self").into()),
        }
    }
}

#[derive(Debug, Clone, Deserialize)]
struct SwitchRule {
    #[serde(rename = "t")]
    operator: SwitchRuleOperator,

    #[serde(rename = "v")]
    value: Variant,

    #[serde(rename = "vt")]
    value_type: RedPropertyType,
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

    async fn dispatch_msg(&self, msg: &MsgHandle, cancel: CancellationToken) -> crate::Result<()> {
        let mut envelopes: SmallVec<[Envelope; 4]> = SmallVec::new();
        for (port, rule) in self.config.rules.iter().enumerate() {
            envelopes.push(Envelope { port, msg: msg.clone() });
            if !self.config.check_all {
                break;
            }
        }

        self.fan_out_many(envelopes, cancel).await
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
