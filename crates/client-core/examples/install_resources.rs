//! Development bootstrap for the reviewed desktop dictionary release.
//! Does not activate a generation or modify an installed input method.
//!
//! 直连 GitHub 不稳的网络里，设 `MSIME_DOWNLOAD_MIRROR`（如 `https://gh-proxy.com`）把每个下载地址变成
//! `<镜像>/<原始 URL>`，与资源包安装器的 `asr_model_mirror` 同一格式；留空即直连。
use msime_client_core::resources::{ResourceSet, ResourceStore};
use std::io::Read;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let root = std::env::args_os()
        .nth(1)
        .ok_or("usage: install_resources <resource-cache-directory>")?;
    let specification: ResourceSet = serde_json::from_str(include_str!(
        "../../../resources/desktop-dictionary.lock.json"
    ))?;
    let mirror = std::env::var("MSIME_DOWNLOAD_MIRROR").unwrap_or_default();
    let client = reqwest::blocking::Client::builder()
        .https_only(true)
        .connect_timeout(std::time::Duration::from_secs(15))
        .timeout(std::time::Duration::from_secs(300))
        .build()?;
    let destination = ResourceStore::new(root).install(&specification, |artifact| {
        eprintln!("Verifying {}", artifact.name);
        let url = if mirror.is_empty() {
            artifact.url.clone()
        } else {
            format!("{}/{}", mirror.trim_end_matches('/'), artifact.url)
        };
        let response = client
            .get(&url)
            .send()
            .and_then(|response| response.error_for_status())
            .map_err(std::io::Error::other)?;
        Ok(Box::new(response) as Box<dyn Read>)
    })?;
    println!("{}", destination.display());
    Ok(())
}
