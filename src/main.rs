use async_trait::async_trait;
use di;
use edgelink::engine::FlowEngine;
use edgelink::registry::{Registry, RegistryImpl};
// use libloading::Library;
use edgelink::Result;
use std::cell::{Cell, RefCell};
use std::rc::Rc;
use std::sync::Arc;
use tokio::sync::{broadcast, Mutex, MutexGuard};
use tokio::{spawn, task, time};
use tokio_util::sync::CancellationToken;

/*
use core::{Plugin, PluginRegistrar};

struct Registrar {
    plugins: Vec<Box<dyn Plugin>>,
}

impl PluginRegistrar for Registrar {
    fn register_plugin(&mut self, plugin: Box<dyn Plugin>) {
        self.plugins.push(plugin);
    }
}

fn main() {
    let mut registrar = Registrar {
        plugins: Vec::new(),
    };

    for path in std::env::args_os().skip(1) {
        // In this code, we never close the shared library - if you need to be able to unload the
        // library, that will require more work.
        let lib = Box::leak(Box::new(Library::new(path).unwrap()));
        // NOTE: You need to do something to ensure you're only loading "safe" code. Out of scope
        // for this code.
        unsafe {
            let func: libloading::Symbol<unsafe extern "C" fn(&mut dyn PluginRegistrar) -> ()> =
                lib.get(b"plugin_entry").unwrap();
            func(&mut registrar);
        }
    }

    for plugin in registrar.plugins {
        plugin.callback1();
        dbg!(plugin.callback2(7));
    }
}

*/

struct Runtime {
    registry: Arc<dyn Registry>,
}

impl Runtime {
    fn new(reg: Arc<dyn Registry>) -> Self {
        Runtime {
            registry: reg.clone(),
        }
    }

    async fn main_flow_task(self: Arc<Self>, _cancel: CancellationToken) {
        let engine = Arc::new(Mutex::new(
            FlowEngine::new(self.registry.clone(), "./flows.json")
                .await
                .unwrap(),
        ));
        let locked = engine.lock().await;
        locked.start().await.unwrap();
    }

    async fn idle_task(self: Arc<Self>, cancel: CancellationToken) {
        loop {
            time::sleep(tokio::time::Duration::from_secs(1)).await;
            if cancel.is_cancelled() {
                println!("Cancelling the idle task...");
                break;
            }
        }
    }

    pub async fn run(self: Arc<Self>, cancel: CancellationToken) -> crate::Result<()> {
        let task1 = tokio::spawn(self.clone().main_flow_task(cancel.child_token()));
        let task2 = tokio::spawn(self.clone().idle_task(cancel.child_token()));
        _ = tokio::join!(task1, task2);
        Ok(())
    }
}

fn register_all_di_services() -> di::ServiceCollection {
    let mut services = di::ServiceCollection::new();
    services
        .add(di::singleton::<dyn Registry, RegistryImpl>().from(|_| Arc::new(RegistryImpl::new())))
        .add(
            di::singleton_as_self::<Runtime>()
                .from(|sp| Arc::new(Runtime::new(sp.get_required::<dyn Registry>()))),
        );
    services
}

#[tokio::main]
async fn main() -> Result<()> {
    // let m = Modal {};
    // m.run().await;
    println!("EdgeLink 1.0");

    let services = register_all_di_services();
    let _provider = services.build_provider()?;
    let sp = Arc::new(_provider);
    let cancel = CancellationToken::new();
    let runtime_cancel_token = cancel.child_token();

    let task = spawn(async move {
        let rt = sp.clone().get_required::<Runtime>();
        rt.run(runtime_cancel_token).await
    });

    tokio::select! {
        _ = tokio:: signal::ctrl_c() => { cancel.cancel() },
        //_ = shutdown_recv.recv() => {},
    }

    match task.await {
        Ok(_) => {
            println!("Async task completed successfully.");
            Ok(())
        }
        Err(err) => {
            eprintln!("Async task failed: {}", err);
            // 在这里可以采取其他操作
            Err(err.into())
        }
    }
}
