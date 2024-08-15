use std::collections::BTreeMap;
use std::str::FromStr;
use std::sync::Arc;
use std::time::Duration;
use tokio::sync::Mutex as TokMutex;

use log;
use tokio_cron_scheduler::{Job, JobScheduler, JobSchedulerError};

use crate::runtime::flow::Flow;
use crate::runtime::model::*;
use crate::runtime::nodes::*;
use crate::EdgeLinkError;

struct CompleteNode {
    base: Arc<BaseFlowNode>,
    scope: BTreeMap<ElementId, Arc<dyn FlowNodeBehavior>>,
}

impl CompleteNode {
    fn create_msg(&self) -> Arc<Msg> {
        let now = crate::utils::time::unix_now().unwrap();
        let payload = Variant::from(now);
        Msg::with_payload(self.base.id, payload)
    }
}

#[async_trait]
impl NodeBehavior for CompleteNode {
    fn id(&self) -> ElementId {
        self.base.id
    }

    fn name(&self) -> &str {
        &self.base.name
    }
}

#[async_trait]
impl FlowNodeBehavior for CompleteNode {
    fn base(&self) -> &BaseFlowNode {
        &self.base
    }

    async fn run(self: Arc<Self>, stop_token: CancellationToken) {}
}

fn new_node(
    _flow: Arc<Flow>,
    base_node: Arc<BaseFlowNode>,
    _config: &RedFlowNodeConfig,
) -> crate::Result<Arc<dyn FlowNodeBehavior>> {
    let node = CompleteNode {
        base: base_node,
        scope: BTreeMap::new(),
    };
    Ok(Arc::new(node))
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "complete", NodeFactory::Flow(new_node))
}
