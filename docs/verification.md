# 验证记录与 Windows 验收

## 本地开发验证（2026-10-16）

环境：Linux、GCC 15.2、Qt 6.11.1，ZzPureTools 固定提交 `9077bbebd972a5a1dc1e7115a6a5b23f0132af20`。Windows 产品限制未取消，使用显式 `ZZAPK_DEVELOPER_CHECKS=ON`。

执行项目目标编译和 CTest：

```sh
cmake --build build/dev --target ZzApkInfo core_tests analyzer_tests window_tests real_tools_tests -j 8
ctest --test-dir build/dev --output-on-failure
```

结果：5/5 CTest 套件通过（startup、window、core、analyzer、real_tools）。界面截图已检查；Xvfb 下的原生 XCB 启动 smoke 退出码为 0。Windows 版本/图标 RC 经 MinGW windres 编译成功；默认非 Windows 配置按预期拒绝。

部署脚本另以替代 windeployqt 的本地路径检查验证资源复制和缺少 CRT DLL 时失败；它没有执行 Windows 部署，不作为 Windows 运行证据。

真实工具检查使用 Linux Android SDK Build Tools 33.0.1；Windows CI 使用项目内 `.exe`。这两者不是同一平台的验证，不相互替代。无 Windows SDK/桌面运行环境时，不宣称 Windows 编译、部署或实机操作已通过。

回归测试曾先验证以下失败再修复：旧分块摘要、空解析实现、异步任务空实现、界面空实现、未转义单引号截断、临时文件未生成、前导连字符被 aapt2 识别为选项。

## Windows 自动检查

运行 `cmake --preset windows-msvc-release`、构建预设、CTest 和 `windows-package`。检查 `real_tools` 不应跳过。部署目录通过 `--smoke` 从另一工作目录启动，且 PATH 不含 Qt SDK；退出码必须为 0。

## Windows 实机验收清单

以下项目需要 Windows 实际运行后勾选，不代表当前已完成：

- [ ] 普通、中文、表情、空格、`-` 开头文件名，在 aapt 和 aapt2 下均成功。
- [ ] 真实 APK（含资源名称、无启动入口、不同 SDK 版本）与原工具输出一致。
- [ ] `Get-FileHash -Algorithm MD5` 与应用显示一致，包含数 GB 文件。
- [ ] 大文件计算时可以移动窗口、取消、换文件；关闭不遗留 aapt 进程或临时副本。
- [ ] 从输出区域、字段和窗口空白处拖入 APK；多个文件、目录和网络 URL 被拒绝。
- [ ] 工具缺失、损坏 APK、无读取权限、临时磁盘不足会显示明确错误，可以恢复操作。
- [ ] 100% / 150% / 200% 缩放下内容可读，键盘 Tab / Ctrl+O / Esc 正常。
- [ ] Windows 浅色/深色主题、窗口置顶切换、最小化和恢复正常。
- [ ] 在未安装 Qt 的机器解压完整发布目录，直接运行正常。
