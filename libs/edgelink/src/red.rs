use serde::{de::Error, Deserialize, Deserializer};
use serde_json::Value;

// type RedNodeID = [char; 16];

#[derive(Debug, serde::Deserialize)]
pub(crate) struct FlowConfig {
    #[serde(deserialize_with = "from_hex")]
    pub(crate) id: u64,

    #[serde(alias = "type")]
    pub(crate) type_name: String,

    pub(crate) label: String,
    pub(crate) disabled: bool,
    pub(crate) info: String,
}

fn from_hex<'de, D>(deserializer: D) -> Result<u64, D::Error>
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
