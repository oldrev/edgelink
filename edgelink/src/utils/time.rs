use chrono::prelude::Utc;
use std::time::{SystemTime, UNIX_EPOCH};

pub fn unix_now() -> i64 {
    let now = SystemTime::now();

    // 获取UNIX Epoch
    let epoch = UNIX_EPOCH;

    // 计算时间间隔
    let duration = now.duration_since(epoch).unwrap();

    // 获取毫秒数
    duration.as_millis() as i64
}

pub fn iso_now() -> String {
    let now = Utc::now();
    now.to_rfc3339()
}

pub fn millis_now() -> String {
    let now = Utc::now();
    now.timestamp_millis().to_string()
}

