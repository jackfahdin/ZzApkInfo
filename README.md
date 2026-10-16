# ZzApkInfo

面向 Windows 的 APK 信息查看工具。参考 QApkInfo 重写，使用 **C++20、Qt 6 和 [ZzPureTools](https://github.com/jackfahdin/ZzPureTools)**；界面全部由 C++ 布局构建，不使用 `.ui` 文件。

## 功能

- 打开或拖入一个 APK，显示应用名称、包名、启动类、版本名称、版本代码和标准文件 MD5。
- 明确选择 aapt / aapt2，切换后自动重新解析当前文件。
- 保留完整 badging 输出，工具警告和错误单独展示；所有结果只读且可复制。
- 后台计算、进度显示、取消、重新解析；可以在分析中打开其他文件。
- 默认窗口置顶，可取消置顶；`Ctrl+O` 打开，`Esc` 取消分析。
- 支持 `ZzApkInfo.exe "D:\Apps\示例.apk"` 启动时打开文件。

解析过程完全在本机进行，不上传 APK，不执行 APK 内容，不需要 Java 或 Android 设备。

## Windows 构建

推荐 Windows 10/11 x64、Visual Studio 2022 17.8+（MSVC 19.38+）、CMake 3.25+、Ninja 和 Qt 6.8+ 的 `msvc2022_64` kit。Qt 需包含 Core、Gui、Widgets、Svg、Concurrent 和 Test；CI 使用 Qt 6.8.3。

在 **x64 Native Tools Command Prompt / Developer PowerShell** 中运行：

```powershell
$env:QT_ROOT = 'D:/Qt/6.8.3/msvc2022_64'
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --parallel 4
ctest --preset windows-msvc-release
cmake --build --preset windows-package
```

可执行文件位于 `build/windows-msvc-release/bin/`，可分发目录位于 `build/windows-msvc-release/dist/`。部署目标使用同一 Qt kit 的 `windeployqt`，复制 Qt DLL、插件、当前 MSVC 工具链的 x64 CRT DLL、aapt/aapt2、图标版本信息及第三方通知。发布目录每次重新生成，仅用于 Release 构建。

框架固定到提交 `9077bbebd972a5a1dc1e7115a6a5b23f0132af20`，首次配置会下载源码。已有同版本源码时可在首次配置追加：

```powershell
cmake --preset windows-msvc-release -DFETCHCONTENT_SOURCE_DIR_ZZPURETOOLS=D:/src/ZzPureTools
```

ZzPureTools 组件静态链接，Qt 动态链接；不需要额外安装框架。源码树不保存生成的版本头、构建文件或机器专用 Qt 路径。自定义本机配置放在被忽略的 `CMakeUserPresets.json` 中。

## 相比原项目修正的行为

原项目把每块内容的 MD5 再拼接计算 MD5，结果不等于文件 MD5；新实现顺序流式读取原始数据，结果可与 PowerShell `Get-FileHash -Algorithm MD5` 比较。计算和兼容复制均在后台执行，内存使用不随 APK 体积线性增长。

新请求会取消旧请求，过期结果不能覆盖新文件。工具使用程序目录内的绝对路径，与启动工作目录无关。非零退出、启动失败、超时、读取失败及文件变化会显示错误；工具输出超过 16 MiB 时主动中止并明确提示。

旧 Windows aapt 对非 ASCII 文件名使用可取消的临时副本，通过 Unicode 工作目录和 ASCII 相对文件名执行；目录持续保留到工具退出。此兼容流程需要临时磁盘空间，aapt2 无需复制。对 `-example.apk` 使用 `./` 前缀，避免被 aapt2 当作命令选项。

不再通过右键按钮暗中切换解析工具，也不在运行时删除 Designer 控件再创建替代品。

## 测试与验证

Qt Test 覆盖标准 MD5、跨块读取、取消、错误、解析转义与本地化、真实子进程错误/超时、快速切换、关闭生命周期和界面拖放。仓库还提供自有最小 APK 样本；Windows 的 `real_tools` 测试直接运行随包工具，覆盖中文/表情/空格及前导连字符文件名。

`.github/workflows/windows.yml` 在 Windows MSVC 上构建、运行测试、部署，并清除 SDK PATH 后启动成品检查；通过后上传 ZIP artifact，不自动发布 Release。Windows 实机验收项见 [验收清单](docs/verification.md)。

当前 Linux 主机仅用于开发验证，**不作为产品支持平台**。需要在 Linux 检查业务与界面时显式启用：

```sh
cmake -S . -B build/dev -G Ninja \
  -DZZAPK_DEVELOPER_CHECKS=ON \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64 \
  -DCMAKE_CXX_COMPILER=g++-15
cmake --build build/dev --target ZzApkInfo core_tests analyzer_tests window_tests --parallel 4
ctest --test-dir build/dev --output-on-failure
```

真实工具测试可另指定 `ZZAPK_REAL_TOOLS_DIR`，该目录需含本机可执行的 `aapt.exe`、`aapt2.exe`（Linux 下可用 SDK 工具的符号链接）。不配置时 Linux 仅运行其余测试；Windows 默认测试仓库内的真实工具。

## 来源

参考项目：[QApkInfo](https://github.com/jackfahdin/QApkInfo)。aapt/aapt2 和图标沿用参考仓库，两个工具的确切 SDK 版本未在原仓库记录；详见 [工具说明](tools/README.md)。ZzPureTools 固定源码及其第三方许可证随部署包保留。Qt 为动态运行库，许可与源码入口见 [Qt 说明](licenses/Qt/README.md)。
