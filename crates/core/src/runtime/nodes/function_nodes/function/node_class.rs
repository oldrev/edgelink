use std::sync::{Arc, Weak};

use rquickjs::{class::Trace, prelude::Opt, Ctx, FromJs, IntoJs, Value};
use tokio::sync::RwLock;
use tokio_util::sync::CancellationToken;

use crate::runtime::js::util;

use super::{EdgelinkError, Envelope, FlowNodeBehavior, FunctionNode, Msg};

#[derive(Clone, Trace)]
#[rquickjs::class(frozen)]
pub(super) struct NodeClass {
    #[qjs(skip_trace)]
    node: Weak<FunctionNode>,
}

#[allow(non_snake_case)]
#[rquickjs::methods]
impl NodeClass {
    // All functions declared in this impl block will be defined on the prototype of the
    // class. This attributes allows you to skip certain functions.
    #[qjs(skip)]
    pub fn new(node: &Arc<FunctionNode>) -> Self {
        NodeClass { node: Arc::downgrade(node) }
    }

    #[qjs(get, rename = "id")]
    fn get_id(&self) -> rquickjs::Result<String> {
        let node = self.node.upgrade().clone().ok_or(rquickjs::Error::UnrelatedRuntime)?;
        Ok(node.base.id.to_string())
    }

    #[qjs(get, rename = "name")]
    fn get_name<'js>(&self, ctx: Ctx<'js>) -> rquickjs::Result<Value<'js>> {
        let node = self.node.upgrade().clone().ok_or(rquickjs::Error::UnrelatedRuntime)?;
        node.base.name.clone().into_js(&ctx)
    }

    #[qjs(get, rename = "outputCount")]
    fn get_output_count(&self) -> rquickjs::Result<usize> {
        let node = self.node.upgrade().clone().ok_or(rquickjs::Error::UnrelatedRuntime)?;
        Ok(node.config.output_count)
    }

    #[qjs(rename = "send")]
    fn send<'js>(self, msgs: Value<'js>, cloning: Opt<bool>, ctx: Ctx<'js>) -> rquickjs::Result<()> {
        let cloning = cloning.unwrap_or(true);
        let async_ctx = ctx.clone();
        if let Err(err) = self.send_msgs_internal(async_ctx, msgs, cloning) {
            // TODO report error
            log::warn!("Failed to send msg(s): {}", err);
        }
        Ok(())
    }

    #[qjs(skip)]
    fn send_msgs_internal<'js>(&self, ctx: Ctx<'js>, msgs: rquickjs::Value<'js>, cloning: bool) -> crate::Result<()> {
        let node = self.node.upgrade().clone().ok_or(rquickjs::Error::UnrelatedRuntime)? as Arc<dyn FlowNodeBehavior>;

        match msgs.type_of() {
            rquickjs::Type::Array => {
                let mut msgs_to_send = Vec::new();
                let ports = msgs.as_array().expect("Must be an array");
                // The first-level array is bound to a port.
                let mut is_first = true;
                for (port, msgs_in_port) in ports.iter().enumerate() {
                    let msgs_in_port: Value<'js> = msgs_in_port?;
                    if let Some(msgs_in_port) = msgs_in_port.as_array() {
                        // The second-level array is msgs in single port.
                        for msg in msgs_in_port.iter() {
                            let msg: Value<'js> = msg?;
                            if msg.is_object() {
                                let msg_value =
                                    if cloning && is_first { util::deep_clone(ctx.clone(), msg)? } else { msg };
                                is_first = false;
                                let envelope =
                                    Envelope { port, msg: Arc::new(RwLock::new(Msg::from_js(&ctx, msg_value)?)) };
                                msgs_to_send.push(envelope);
                            }
                        }
                    } else if msgs_in_port.is_object() {
                        // This port has only one msg
                        let msg_value = if cloning && is_first {
                            util::deep_clone(ctx.clone(), msgs_in_port)?
                        } else {
                            msgs_in_port
                        };
                        is_first = false;
                        let envelope = Envelope { port, msg: Arc::new(RwLock::new(Msg::from_js(&ctx, msg_value)?)) };
                        msgs_to_send.push(envelope);
                    } else {
                        log::warn!("Unknown msg type: {}", port);
                    }
                }

                let cancel = CancellationToken::new();
                let async_node = node.clone();
                ctx.spawn(async move {
                    match async_node.fan_out_many(&msgs_to_send, cancel).await {
                        Ok(_) => {}
                        Err(err) => log::error!("Failed to send msg in function node: {}", err),
                    }
                });
            }

            rquickjs::Type::Object => {
                let msg_to_send = Arc::new(RwLock::new(Msg::from_js(&ctx, msgs)?));
                let envelope = Envelope { port: 0, msg: msg_to_send };
                // FIXME
                let cancel = CancellationToken::new();
                let async_node = node.clone();
                ctx.spawn(async move {
                    match async_node.fan_out_one(&envelope, cancel).await {
                        Ok(_) => {}
                        Err(err) => log::error!("Failed to send msg in function node: {}", err),
                    }
                });
            }

            _ => {
                return Err(EdgelinkError::InvalidOperation(format!("Unsupported: {:?}", msgs.type_of())).into());
            }
        }
        Ok(())
    }
}
