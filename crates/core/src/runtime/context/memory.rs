use std::collections::HashMap;

use async_trait::async_trait;
// use itertools::Itertools;
use tokio::sync::RwLock;

use super::{EdgelinkError, ElementId, Variant};
use crate::runtime::context::*;
use crate::Result;

inventory::submit! {
    ProviderMetadata { type_: "memory", factory: MemoryContextStore::build }
}

struct MemoryContextStore {
    name: String,
    scopes: RwLock<HashMap<String, VariantObjectMap>>,
}

impl MemoryContextStore {
    fn build(name: String, _options: Option<&ContextStoreOptions>) -> crate::Result<Box<dyn ContextStore>> {
        let this = MemoryContextStore { name, scopes: RwLock::new(HashMap::new()) };
        Ok(Box::new(this))
    }
}

#[async_trait]
impl ContextStore for MemoryContextStore {
    async fn name(&self) -> &str {
        &self.name
    }

    async fn open(&self) -> Result<()> {
        // No-op for in-memory store
        Ok(())
    }

    async fn close(&self) -> Result<()> {
        // No-op for in-memory store
        Ok(())
    }

    async fn get_one(&self, scope: &str, key: &str) -> Result<Variant> {
        let scopes = self.scopes.read().await;
        if let Some(scope_map) = scopes.get(scope) {
            if let Some(value) = scope_map.get(key) {
                return Ok(value.clone());
            }
        }
        Err(EdgelinkError::OutOfRange.into())
    }

    async fn get_many(&self, scope: &str, keys: &[&str]) -> Result<Vec<Variant>> {
        let scopes = self.scopes.read().await;
        if let Some(scope_map) = scopes.get(scope) {
            let mut result = Vec::new();
            for key in keys {
                if let Some(value) = scope_map.get(*key) {
                    result.push(value.clone());
                }
            }
            return Ok(result);
        }
        Err(EdgelinkError::OutOfRange.into())
    }

    async fn get_keys(&self, scope: &str) -> Result<Vec<String>> {
        let scopes = self.scopes.read().await;
        if let Some(scope_map) = scopes.get(scope) {
            return Ok(scope_map.keys().cloned().collect::<Vec<_>>());
        }
        Err(EdgelinkError::OutOfRange.into())
    }

    async fn set_one(&self, scope: &str, key: &str, value: Variant) -> Result<()> {
        let mut scopes = self.scopes.write().await;
        let scope_map = scopes.entry(scope.to_string()).or_insert_with(VariantObjectMap::new);
        let _ = scope_map.insert(key.to_string(), value);
        Ok(())
    }

    async fn set_many(&self, scope: &str, pairs: &[(&str, &Variant)]) -> Result<()> {
        let mut scopes = self.scopes.write().await;
        let scope_map = scopes.entry(scope.to_string()).or_insert_with(VariantObjectMap::new);
        for (key, value) in pairs {
            let _ = scope_map.insert(key.to_string(), (*value).clone());
        }
        Ok(())
    }

    async fn remove_one(&self, scope: &str, key: &str) -> Result<Variant> {
        let mut scopes = self.scopes.write().await;
        if let Some(scope_map) = scopes.get_mut(scope) {
            if let Some(value) = scope_map.remove(key) {
                return Ok(value);
            } else {
                return Err(EdgelinkError::OutOfRange.into());
            }
        }
        Err(EdgelinkError::OutOfRange.into())
    }

    async fn delete(&self, scope: &str) -> Result<()> {
        let mut scopes = self.scopes.write().await;
        scopes.remove(scope);
        Ok(())
    }

    async fn clean(&self, _active_nodes: &[ElementId]) -> Result<()> {
        /*
        let mut items = self.items.write().await;
        let scopes = active_nodes. scope.parse::<ElementId>();
        items.retain(|scope, _| active_nodes.contains(&scope));
        Ok(())
        */
        todo!()
    }
}
