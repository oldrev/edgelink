use crate::EdgeLinkError;
use std::time::Duration;
use tokio_util::sync::CancellationToken;

pub async fn delay(dur: Duration, cancel: CancellationToken) -> crate::Result<()> {
    tokio::select! {
        _ = cancel.cancelled() => {
            // 取消 sleep_task 任务
            Err(EdgeLinkError::TaskCancelled.into())
        }
        _ = tokio::time::sleep(dur) => {
            // Long work has completed
            Ok(())
        }
    }
}

pub async fn delay_millis(millis: u64, cancel: CancellationToken) -> crate::Result<()> {
    delay(Duration::from_millis(millis), cancel).await
}
