use std::sync::Arc;
use tokio::sync::RwLock as TokRwLock;
use tokio::time;
use tokio_util::sync::CancellationToken;
// use libloading::Library;

use edgelink::runtime::engine::FlowEngine;
use edgelink::runtime::registry::{Registry, RegistryImpl};
use edgelink::Result;

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
    engine: TokRwLock<Option<Arc<FlowEngine>>>,
}

impl Runtime {
    fn new(reg: Arc<dyn Registry>) -> Self {
        Runtime {
            registry: reg.clone(),
            engine: TokRwLock::new(None),
        }
    }

    async fn main_flow_task(self: Arc<Self>, cancel: CancellationToken) {
        let mut engine_holder = self.engine.write().await;
        let engine = FlowEngine::new(self.registry.clone(), "./flows.json")
            .await
            .unwrap();
        *engine_holder = Option::Some(engine.clone());
        engine.start().await.unwrap();
        let wait_cancel = cancel;
        wait_cancel.cancelled().await;
        let _ = engine.stop().await;
        println!("The flows engine stopped.");
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
        let task1 = tokio::task::spawn(self.clone().main_flow_task(cancel.clone()));
        let task2 = tokio::task::spawn(self.clone().idle_task(cancel.clone()));
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

async fn run_main_task(sp: &di::ServiceProvider, cancel: CancellationToken) -> crate::Result<()> {
    let rt = sp.clone().get_required::<Runtime>();
    rt.run(cancel.clone()).await
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

    let runtime_cancel_token = cancel.clone();

    tokio::select! {
        _ = tokio:: signal::ctrl_c() => {
            println!("CTRL-C is pressed, cancelling all tasks...");
            cancel.cancel()
        },
        _ = run_main_task(&sp, runtime_cancel_token) =>  {
            println!("Main task stopped. This should not happen!")
        },
    }

    /*
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
    */

    Ok(())
}
