use std::collections::BTreeMap;
use std::sync::{Arc, Weak};
use tokio::sync::mpsc;
use tokio::sync::Mutex as TokMutex;
use tokio::sync::RwLock as TokRwLock;
use tokio_util::sync::CancellationToken;

use crate::runtime::engine::FlowEngine;
use crate::runtime::model::{ElementId, Msg, Port, PortWire, Variant};
use crate::runtime::nodes::*;
use crate::runtime::red::json::{RedFlowConfig, RedFlowNodeConfig};
use crate::runtime::registry::Registry;
use crate::EdgeLinkError;

const NODE_MSG_CHANNEL_CAPACITY: usize = 32;

struct FlowState {
    nodes: BTreeMap<ElementId, Arc<dyn FlowNodeBehavior>>,
    nodes_ordering: Vec<ElementId>,
    _context: Variant,
    _engine: Weak<FlowEngine>,
}

pub type FlowNodeTask = tokio::task::JoinHandle<()>;

struct FlowShared {
    state: TokRwLock<FlowState>,
}

pub struct Flow {
    pub id: ElementId,
    pub label: String,
    pub disabled: bool,

    pub stop_token: CancellationToken,
    pub stopped_tx: mpsc::Sender<()>,
    stopped_rx: TokMutex<mpsc::Receiver<()>>,

    shared: Arc<FlowShared>,
}

impl Flow {
    pub fn id(&self) -> ElementId {
        self.id
    }

    pub fn label(&self) -> &str {
        &self.label
    }

    pub fn disabled(&self) -> bool {
        self.disabled
    }

    /// 从来源节点的指定单个端口发送多个消息
    pub async fn fan_out_single_port(
        &self,
        src_node_id: ElementId,
        src_port_index: usize,
        msgs: &[Arc<Msg>],
        _cancel: CancellationToken,
    ) -> crate::Result<()> {
        let state = self.shared.state.read().await;
        let src_node = &state.nodes[&src_node_id];
        if src_port_index >= src_node.base().ports.len() {
            return Err(
                crate::EdgeLinkError::InvalidOperation("Invalid port index".to_string()).into(),
            );
        }
        let port = &src_node.base().ports[src_port_index];

        let mut msg_sent = false;
        for wire in port.wires.iter() {
            //let dest_node = &state.nodes[dest_node_id];
            for msg in msgs.iter() {
                let msg_to_send: Arc<Msg> = if msg_sent {
                    Arc::new(msg.as_ref().clone())
                } else {
                    msg.clone()
                };
                wire.msg_sender.send(msg_to_send).await?;
                debug_assert!(!wire.msg_sender.is_closed());
                msg_sent = true;
            }
        }
        Ok(())
    }

    pub async fn fan_out_all(
        &self,
        _port_msgs: Vec<Option<Vec<Arc<Msg>>>>,
        _cancel: CancellationToken,
    ) -> crate::Result<()> {
        Ok(())
    }

    /// 从指定的端口扇出
    pub async fn fan_out_port(
        &self,
        port_index: usize,
        msg: Arc<Msg>,
        _cancel: CancellationToken,
    ) -> crate::Result<()> {
        let state = self.shared.state.read().await;
        let source_node = &state.nodes[&msg.birth_place()];
        let port = source_node
            .base()
            .ports
            .get(port_index)
            .ok_or("Failed to get ports")?;

        for wire in port.wires.iter() {
            let _dest_node =
                Weak::upgrade(&wire.target_node).ok_or("Failed to get node id in port")?;
            let _msg_to_send = msg.clone();
            //dest_node.fan_in(msg_to_send, cancel.clone()).await?;
        }

        Ok(())
    }

    /*
    /// 这里值传递 Vec 要不要优化下
    async fn deliver_msgs(
        &self,
        envelopes: Vec<crate::msg::Envelope>,
        cancel: CancellationToken,
    ) -> crate::Result<()> {
        let state = self.shared.state.read().await;

        for envelope in envelopes.iter() {
            let src_node = &state.nodes[&envelope.src_node_id];
            let src_port = src_node.base().ports.iter().nth(envelope.src_port_index).unwrap();
            let dest_node = &state.nodes[&envelope.dest_node_id];
            let msg_to_send: Arc<Msg> = if envelope.clone_msg {
                Arc::new(envelope.msg.as_ref().clone())
            } else {
                envelope.msg.clone()
            };
            //dest_node.fan_in(msg_to_send, cancel.clone()).await?;
        }

        Ok(())
    }
    */

    pub(crate) async fn new(
        engine: Arc<FlowEngine>,
        flow_config: &RedFlowConfig,
        reg: Arc<dyn Registry>,
    ) -> crate::Result<Arc<Self>> {
        let (stopped_tx, mut stopped_rx) = mpsc::channel(1);
        let flow: Arc<Flow> = Arc::new(Flow {
            id: flow_config.id,
            label: flow_config.label.clone(),
            disabled: flow_config.disabled.unwrap_or(false),
            stopped_rx: TokMutex::new(stopped_rx),
            stopped_tx,
            shared: Arc::new(FlowShared {
                state: TokRwLock::new(FlowState {
                    _engine: Arc::downgrade(&engine),
                    nodes: BTreeMap::new(),
                    nodes_ordering: Vec::new(),
                    _context: Variant::empty_object(),
                }),
            }),
            stop_token: CancellationToken::new(),
        });

        let scoped_flow = flow.clone();
        {
            let mut state = scoped_flow.shared.state.write().await;

            for node_config in flow_config.nodes.iter() {
                if let Some(meta_node) = reg.get(&node_config.type_name) {
                    let node = match meta_node.factory {
                        NodeFactory::Flow(factory) => {
                            let base_flow_node = scoped_flow
                                .clone()
                                .new_base_flow_node(&state, node_config)?;
                            factory(scoped_flow.clone(), base_flow_node, node_config)
                        }
                        _ => {
                            return Err(EdgeLinkError::NotSupported(format!(
                                "Can not found flow node factory for Node(id={0}, type='{1}'",
                                flow_config.id, flow_config.type_name
                            ))
                            .into())
                        }
                    };
                    state.nodes_ordering.push(node.id());

                    state.nodes.insert(node_config.id, node);
                }
            }
        }

        Ok(flow)
    }

    pub(crate) async fn start(self: Arc<Self>) -> crate::Result<()> {
        let state = self.shared.state.write().await;
        println!("-- Starting Flow (id={0})...", self.id);
        // 启动是按照节点依赖顺序的正序
        for node_id in state.nodes_ordering.iter() {
            let node = state.nodes[node_id].clone();
            println!("---- Starting Node (id='{0}')...", node.id());
            // Start the async-task of each flow node
            let node_to_run = node.clone();
            let child_stop_token = self.stop_token.child_token();
            tokio::task::spawn(async move { node_to_run.run(child_stop_token).await });
        }
        Ok(())
    }

    pub(crate) async fn stop(self: Arc<Self>) -> crate::Result<()> {
        let state = self.shared.state.write().await;
        println!("-- Stopping Flow (id={0})...", self.id);
        self.stop_token.cancel();
        drop(&self.stopped_tx);
        let stopped_rx = &mut self.stopped_rx.lock().await;
        let _ = stopped_rx.recv().await;
        Ok(())
    }

    fn new_base_flow_node(
        self: Arc<Self>,
        state: &FlowState,
        node_config: &RedFlowNodeConfig,
    ) -> crate::Result<Arc<BaseFlowNode>> {
        let mut ports = Vec::new();
        let (tx_root, rx) = mpsc::channel(NODE_MSG_CHANNEL_CAPACITY);
        // Convert the Node-RED wires elements to ours
        for red_port in node_config.wires.iter() {
            let mut wires = Vec::new();
            for nid in red_port.node_ids.iter() {
                let node_entry = state.nodes.get(nid).ok_or("Can not found target node")?;
                let tx = node_entry.base().msg_tx.to_owned();
                let pw = PortWire {
                    target_node_id: *nid,
                    target_node: Arc::downgrade(node_entry),
                    msg_sender: tx,
                };
                wires.push(pw);
            }
            let port = Port { wires };
            ports.push(port);
        }

        Ok(Arc::new(BaseFlowNode {
            id: node_config.id,
            flow: Arc::downgrade(&self),
            name: node_config.name.clone(),
            msg_tx: tx_root,
            msg_rx: MsgReceiverHolder::new(rx),
            ports,
        }))
    }
}