# 本机便捷入口。平台构建的权威说明在 platforms/macos/README.md 与 scripts/verify-local.sh；
# 这里只把最常用的打包命令收成一条。

# Sparkle 2.9.6 的解压目录（里面要有 Sparkle.framework），package-release.sh 必需。
MSIME_SPARKLE_ROOT ?= $(abspath target/tooling/sparkle)
# 包版本号；留空则用 platforms/macos/version.txt。
VERSION ?=
# 版本 id（shared/contracts/editions.json），如 full,wubi；留空只打 full。
EDITIONS ?=

pkg_args = $(if $(EDITIONS),--editions $(EDITIONS),) $(VERSION)

# 打 universal DMG 到 target/macos-package/dist/，旁边生成 SHA256SUMS。
# 签名/公证凭据见脚本注释（MACOS_SIGNING_IDENTITY、APPLE_ID 等），不设则 ad-hoc。
.PHONY: macos-dmg
macos-dmg:
	MSIME_SPARKLE_ROOT="$(MSIME_SPARKLE_ROOT)" platforms/macos/package-release.sh $(pkg_args)
