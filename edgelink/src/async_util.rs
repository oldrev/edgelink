use std::time::Duration;
use tokio::time::sleep;
use tokio_util::sync::CancellationToken;

pub async fn delay(dur: Duration, cancel: CancellationToken) {
    tokio::select! {
        _ = cancel.cancelled() => {
            // 取消 sleep_task 任务
        }
        _ = tokio::time::sleep(dur) => {
            // Long work has completed
        }
    }
}

pub async fn delay_millis(millis: u64, cancel: CancellationToken) {
    delay(Duration::from_millis(millis), cancel).await
}
