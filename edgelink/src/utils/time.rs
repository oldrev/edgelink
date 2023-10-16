use std::time::{SystemTime, UNIX_EPOCH};

pub fn unix_now() -> crate::Result<i64> {
    let now = SystemTime::now();

    // 获取UNIX Epoch
    let epoch = UNIX_EPOCH;

    // 计算时间间隔
    let duration = now.duration_since(epoch)?;

    // 获取毫秒数
    Ok(duration.as_millis() as i64)
}
