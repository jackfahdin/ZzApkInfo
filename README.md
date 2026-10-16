# ZzApkInfo

[![Windows 持续构建](https://github.com/jackfahdin/ZzApkInfo/actions/workflows/windows.yml/badge.svg)](https://github.com/jackfahdin/ZzApkInfo/actions/workflows/windows.yml)

面向 Windows 的 APK 信息查看工具，使用 **C++20、Qt 6 和 [ZzPureTools](https://github.com/jackfahdin/ZzPureTools)**。

## 下载

[Windows x64 ZIP](https://github.com/jackfahdin/ZzApkInfo/releases/download/continuous-build/ZzApkInfo-continuous-windows-x64.zip) · [SHA-256 校验文件](https://github.com/jackfahdin/ZzApkInfo/releases/download/continuous-build/ZzApkInfo-continuous-windows-x64.zip.sha256) · [预发布页面](https://github.com/jackfahdin/ZzApkInfo/releases/tag/continuous-build)

持续构建使用 **Release** 配置。主分支通过编译、测试和解压启动检查后，自动更新 `continuous-build` 预发布。

## 功能

- 打开或拖入 APK，查看应用名称、包名、类名、版本和文件 MD5。
- 右键点击「选择·aapt」切换 aapt / aapt2，并自动重新解析当前文件。
- 显示完整解析输出及工具提示，结果可选择、复制。
- 后台计算 MD5 并显示进度，切换文件后自动更新结果。
- 窗口默认置顶；支持 `Ctrl+O` 打开文件。

解析过程在本机完成，不上传 APK，不需要 Java 或 Android 设备。

## 使用

解压发布包后运行 `ZzApkInfo.exe`，将 APK 拖入窗口或左键点击「选择·aapt」。也可以通过命令行打开文件：

```powershell
ZzApkInfo.exe "D:\Apps\示例.apk"
```

请保留发布目录中的 aapt/aapt2、Qt DLL 和插件文件。

## Windows 构建

需要 Windows 10/11 x64、Visual Studio 2022 17.8+（MSVC 19.38+）、Git、CMake 3.25+、Ninja 和 Qt 6.8+ 的 `msvc2022_64` kit。Qt 需包含 Core、Gui、Widgets、Svg、Concurrent 和 Test；CI 使用 Qt 6.8.3。

克隆仓库及子模块：

```powershell
git clone --recurse-submodules https://github.com/jackfahdin/ZzApkInfo.git
cd ZzApkInfo
```

已有仓库可初始化或同步子模块：

```powershell
git submodule update --init --recursive
```

在 **x64 Developer PowerShell** 中设置 Qt 路径并构建：

```powershell
$env:QT_ROOT = 'D:/Qt/6.8.3/msvc2022_64'
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --parallel 4
ctest --preset windows-msvc-release
```

可执行文件位于 `build/windows-msvc-release/bin/`。

## Windows 打包

在 x64 Developer PowerShell 中运行统一打包脚本：

```powershell
.\.github\scripts\package-windows.ps1
```

脚本会构建 Release 程序、调用 CPack 生成 ZIP，并检查解压后的程序能否独立启动。可以用 `-BuildDirectory` 指定已配置的构建目录。

也可以在完成 Release 编译后直接使用 CPack：

```powershell
cpack --preset windows-msvc-release
```

产物统一放在项目根目录的 `dist` 下：

```text
dist/
├── ZzApkInfo-1.0.0-windows-x64/
├── ZzApkInfo-1.0.0-windows-x64.zip
└── ZzApkInfo-1.0.0-windows-x64.zip.sha256
```

目录和文件名中的版本号自动读取项目配置，ZIP 内包含同名顶层目录。仅打包程序及 aapt/aapt2、实际依赖的 Qt/MSVC DLL、Windows 平台和 ICO 图标插件，以及许可证；不复制 Qt SDK、未使用的插件、头文件或静态库。

## 依赖与测试

[ZzPureTools](https://github.com/jackfahdin/ZzPureTools) 以 Git 子模块保存在 `third_party/ZzPureTools`，版本由仓库中的子模块提交记录固定。

测试覆盖 MD5、元数据解析、取消与错误处理、文件切换、界面拖放，以及真实 aapt/aapt2 对特殊文件名的解析。Windows CI 执行构建、测试、打包和解压启动检查。主分支推送成功后更新持续预发布；PR 和手动构建只验证并保留 Actions 产物。

第三方许可证随发布包保留，Qt 许可说明见 [licenses/Qt](licenses/Qt/README.md)。
