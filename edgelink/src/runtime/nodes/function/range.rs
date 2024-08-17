use std::str::FromStr;
use std::sync::Arc;

use log;

use crate::runtime::flow::Flow;
use crate::runtime::model::*;
use crate::runtime::nodes::*;

struct RangeNode {
    base: Arc<BaseFlowNode>,

    action: RangeAction,
    round: Option<f64>,
    minin: f64,
    maxin: f64,
    minout: f64,
    maxout: f64,
    property: String,
}

impl RangeNode {
    fn do_range(self: Arc<Self>) {

    }
}

#[async_trait]
impl NodeBehavior for RangeNode {
    fn id(&self) -> ElementId {
        self.base.id
    }

    fn name(&self) -> &str {
        &self.base.name
    }
}

#[derive(Debug)]
enum RangeAction {
    Scale,
    Drop,
    Clamp,
    Roll,
}

impl FromStr for RangeAction {
    type Err = ();

    fn from_str(input: &str) -> Result<RangeAction, Self::Err> {
        match input.to_lowercase().as_str() {
            "scale" => Ok(RangeAction::Scale),
            "drop" => Ok(RangeAction::Drop),
            "clamp" => Ok(RangeAction::Clamp),
            "roll" => Ok(RangeAction::Roll),
            _ => Err(()),
        }
    }
}

#[async_trait]
impl FlowNodeBehavior for RangeNode {
    fn base(&self) -> &BaseFlowNode {
        &self.base
    }

    async fn run(self: Arc<Self>, stop_token: CancellationToken) {
        let flow_ref = Weak::upgrade(&self.base().flow).unwrap();
        while !stop_token.is_cancelled() {
            match self.wait_for_msg(stop_token.clone()).await {
                Ok(msg) => {
                    flow_ref
                        .fan_out_single_port(self.base.id, 0, &[msg], stop_token.clone())
                        .await
                        .unwrap(); //FIXME
                }
                Err(ref err) => {
                    log::error!("Error: \n{:#?}", err);
                    break;
                }
            }
        }

        let rx = &mut self.base().msg_rx.rx.lock().await;
        rx.close();
        log::debug!("DebugNode process() task has been terminated.");
    }

}

fn new_node(
    _flow: Arc<Flow>,
    base_node: Arc<BaseFlowNode>,
    _config: &RedFlowNodeConfig,
) -> crate::Result<Arc<dyn FlowNodeBehavior>> {
    let action = _config
        .json
        .get("action")
        .and_then(|jv| jv.as_str())
        .and_then(|value| RangeAction::from_str(value).ok())
        .ok_or(EdgeLinkError::NotSupported(
            "Bad range node action".to_string(),
        ))?;

    let node = RangeNode {
        base: base_node,
        action: action,

        round: _config
            .json
            .get("round")
            .and_then(|jv| jv.as_str())
            .and_then(|value| value.parse::<f64>().ok()),

        minin: _config
            .json
            .get("minin")
            .and_then(|jv| jv.as_str())
            .and_then(|value| value.parse::<f64>().ok())
            .unwrap_or(0.0),

        maxin: _config
            .json
            .get("maxin")
            .and_then(|jv| jv.as_str())
            .and_then(|value| value.parse::<f64>().ok())
            .unwrap_or(0.0),

        minout: _config
            .json
            .get("minout")
            .and_then(|jv| jv.as_str())
            .and_then(|value| value.parse::<f64>().ok())
            .unwrap_or(0.0),

        maxout: _config
            .json
            .get("maxout")
            .and_then(|jv| jv.as_str())
            .and_then(|value| value.parse::<f64>().ok())
            .unwrap_or(0.0),

        property: _config
            .json
            .get("property")
            .and_then(|jv| jv.as_str())
            .and_then(|v| {
                if v.is_empty() {
                    None
                } else {
                    Some(v.to_string())
                }
            })
            .unwrap_or("payload".to_string()),
    };
    Ok(Arc::new(node))
}

inventory::submit! {
    BuiltinNodeDescriptor::new(NodeKind::Flow, "range", NodeFactory::Flow(new_node))
}
