use edgelink::{Plugin, PluginRegistrar};

struct PluginB;

impl Plugin for PluginB {
    fn callback1(&self) {
        println!("PluginB::callback1")
    }

    fn callback2(&self, i: i32) -> i32 {
        println!("PluginB::callback2");
        i - 1
    }
}

#[no_mangle]
pub fn plugin_entry(registrar: &mut dyn PluginRegistrar) {
    registrar.register_plugin(Box::new(PluginB));
}
