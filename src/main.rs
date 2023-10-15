use async_trait::async_trait;
use di::ServiceRef;
use edgelink::engine::FlowEngine;
use edgelink::registry::Registry;
// use libloading::Library;
use edgelink::Result;
use std::cell::{Cell, RefCell};
use std::sync::Arc;
use tokio::sync::{Mutex, MutexGuard};
use tokio::{spawn, task, time};

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

async fn start() -> Result<()> {
    let reg = Registry::new()?;
    let engine = Arc::new(Mutex::new(FlowEngine::new(&reg, "./flows.json").await?));
    spawn(async move {
        //
        let locked = engine.lock().await;
        locked.start().await
    })
    .await?
}

#[tokio::main]
async fn main() -> Result<()> {
    // let m = Modal {};
    // m.run().await;
    println!("EdgeLink 1.0");

    let task = spawn(async { start().await });

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
    // loop { time::sleep(tokio::time::Duration::from_secs(1)).await; }
}
