use serde::{de::Error, Deserialize, Deserializer}; // 1.0.94
use serde_json; // 1.0.40

#[derive(Debug, PartialEq)]
struct ElementID(u64);

impl<'de> Deserialize<'de> for ElementID {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s: &str = Deserialize::deserialize(deserializer)?;
        // do better hex decoding than this
        u64::from_str_radix(&s[2..], 16)
            .map(Account)
            .map_err(D::Error::custom)
    }
}
