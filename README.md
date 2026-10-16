# ZzApkInfo

面向 Windows 的 APK 信息查看工具，使用 **C++20、Qt 6 和 [ZzPureTools](https://github.com/jackfahdin/ZzPureTools)**。

## 功能

- 打开或拖入 APK，查看应用名称、包名、启动类、版本名称、版本代码和文件 MD5。
- 支持 aapt / aapt2，切换工具后自动重新解析当前文件。
- 显示完整解析输出，工具警告和错误单独展示；结果可选择、复制。
- 支持后台分析、进度显示、取消和重新解析。
- 可切换窗口置顶；`Ctrl+O` 打开文件，`Esc` 取消分析。

解析过程在本机完成，不上传 APK，不需要 Java 或 Android 设备。

## 使用

解压发布包后运行 `ZzApkInfo.exe`，将 APK 拖入窗口或点击「打开 APK」。也可以通过命令行打开文件：

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
cmake --build --preset windows-package
```

可执行文件位于 `build/windows-msvc-release/bin/`，完整发布目录位于 `build/windows-msvc-release/dist/`。打包目标仅用于 Release 构建，每次重新生成发布目录。

## 依赖与测试

[ZzPureTools](https://github.com/jackfahdin/ZzPureTools) 以 Git 子模块保存在 `third_party/ZzPureTools`，版本由仓库中的子模块提交记录固定。

测试覆盖 MD5、元数据解析、取消与错误处理、文件切换、界面拖放，以及真实 aapt/aapt2 对特殊文件名的解析。Windows CI 执行构建、测试、打包和启动检查，通过后上传发布包。

第三方许可证随发布包保留，Qt 许可说明见 [licenses/Qt](licenses/Qt/README.md)。
