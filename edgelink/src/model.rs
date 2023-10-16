use serde::{de::Error, Deserialize, Deserializer};
use serde_json::Value as JsonValue;

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Copy, Clone)]
pub struct ElementID(u64);

impl<'de> Deserialize<'de> for ElementID {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let v: JsonValue = Deserialize::deserialize(deserializer)?;

        match v {
            JsonValue::String(r) => Ok(u64::from_str_radix(r.as_str(), 16)
                .map(ElementID)
                .map_err(D::Error::custom)),
            JsonValue::Number(num) => {
                if let Some(u64v) = num.as_u64() {
                    return Ok(ElementID(u64v));
                } else {
                    Err(num).map_err(D::Error::custom)
                }
            }
            other => Err(other).map_err(D::Error::custom),
        }?
    }
}

impl std::fmt::Display for ElementID {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{:016x}", self.0)
    }
}
