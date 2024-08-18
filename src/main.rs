use clap::Parser;
use std::env;
use dirs_next::home_dir;
use std::process;
use std::sync::Arc;
use tokio::runtime::Builder;
use tokio::sync::RwLock as TokRwLock;
use tokio::time;
use tokio_util::sync::CancellationToken;

// use libloading::Library;

use edgelink::runtime::engine::FlowEngine;
use edgelink::runtime::registry::{Registry, RegistryImpl};
use edgelink::Result;

/// Simple program to greet a person
#[derive(Parser, Debug, Clone)]
#[command(version, about, long_about = None)]
struct EdgeLinkArgs {
    /// Path of the 'flows.json' file
    #[arg(short, long, default_value_t = default_flows_path())]
    flows_path: String,
}

fn default_flows_path() -> String {
    match home_dir() {
        Some(path) => path
            .join(".node-red")
            .join("flows.json")
            .to_string_lossy()
            .to_string(),
        None => "".to_string(),
    }
}

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
pub(crate) fn log_init() {
    log4rs::init_file("log.toml", Default::default()).unwrap();
}

struct Runtime {
    args: Arc<EdgeLinkArgs>,
    registry: Arc<dyn Registry>,
    engine: TokRwLock<Option<Arc<FlowEngine>>>,
}

impl Runtime {
    fn new(elargs: Arc<EdgeLinkArgs>, reg: Arc<dyn Registry>) -> Self {
        Runtime {
            args: elargs.clone(),
            registry: reg.clone(),
            engine: TokRwLock::new(None),
        }
    }

    async fn main_flow_task(self: Arc<Self>, cancel: CancellationToken) -> crate::Result<()> {
        let mut engine_holder = self.engine.write().await;
        log::info!("Loading flows file: {}", &self.args.flows_path);
        let engine = FlowEngine::new(self.registry.clone(), &self.args.flows_path).await?;
        *engine_holder = Option::Some(engine.clone());
        engine.start().await?;
        let wait_cancel = cancel;
        wait_cancel.cancelled().await;
        engine.stop().await?;
        log::info!("The flows engine stopped.");
        Ok(())
    }

    async fn idle_task(self: Arc<Self>, cancel: CancellationToken) -> crate::Result<()> {
        loop {
            time::sleep(tokio::time::Duration::from_secs(1)).await;
            if cancel.is_cancelled() {
                log::info!("Cancelling the idle task...");
                break;
            }
        }
        Ok(())
    }

    pub async fn run(self: Arc<Self>, cancel: CancellationToken) -> crate::Result<()> {
        let task1 = tokio::task::spawn(self.clone().main_flow_task(cancel.clone()));
        let task2 = tokio::task::spawn(self.clone().idle_task(cancel.clone()));
        let result = tokio::join!(task1, task2);
        if result.0.is_err() {
            log::error!("MainFlowTask failure");
            return Err(
                edgelink::EdgeLinkError::NotSupported("Bad main flow task".to_string()).into(),
            );
        }
        if result.1.is_err() {
            log::error!("IdleTask failure");
            return Err(
                edgelink::EdgeLinkError::NotSupported("Bad main flow task".to_string()).into(),
            );
        }
        Ok(())
    }
}

fn register_all_di_services(args: EdgeLinkArgs) -> di::ServiceCollection {
    let mut services = di::ServiceCollection::new();
    services
        .add(di::singleton::<dyn Registry, RegistryImpl>().from(|_| Arc::new(RegistryImpl::new())))
        .add(di::singleton_as_self::<Runtime>().from(move |sp| {
            Arc::new(Runtime::new(
                Arc::new(args.clone()),
                sp.get_required::<dyn Registry>(),
            ))
        }));
    services
}

async fn run_main_task(sp: &di::ServiceProvider, cancel: CancellationToken) -> crate::Result<()> {
    let rt = sp.clone().get_required::<Runtime>();
    rt.run(cancel.clone()).await
}

fn app_main() -> edgelink::Result<()> {
    log_init();

    // let m = Modal {};
    // m.run().await;
    log::info!("EdgeLink {}", env!("CARGO_PKG_VERSION"));
    log::info!("==========================================================\n");

    let elargs = EdgeLinkArgs::parse();
    let services = register_all_di_services(elargs);

    let _provider = services.build_provider()?;
    let sp = Arc::new(_provider);

    // 使用 Builder 创建一个带有自定义线程池的 Tokio 运行时
    let runtime = Builder::new_multi_thread()
        .worker_threads(4) // 设置线程池大小
        .enable_all()
        .build()
        .unwrap();

    runtime.block_on(async {
        let cancel = CancellationToken::new();

        let runtime_cancel_token = cancel.clone();

        tokio::spawn(async move {
            tokio::select! {
                _ = tokio:: signal::ctrl_c() => {
                    println!("CTRL-C is pressed, cancelling all tasks...");
                    cancel.cancel()
                },
                _ = run_main_task(&sp, runtime_cancel_token) =>  {
                    log::error!("Main task stopped. This should not happen!")
                },
            }
        })
        .await
        .unwrap();
    });
    Ok(())
}

fn main() {
    if let Err(err) = app_main() {
        eprintln!("Application error: {}", err);
        process::exit(-1);
    }
}
