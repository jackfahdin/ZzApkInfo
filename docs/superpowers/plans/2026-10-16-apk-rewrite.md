# ZzApkInfo 实现计划

目标：完整替换 QApkInfo 功能，使用 C++20 和 ZzPureTools 纯代码界面，分类提交。

架构：业务核心独立于 Widgets，窗口仅处理展示和交互。外部工具使用异步进程，文件哈希使用可取消后台任务。

技术栈：CMake、Qt 6.8+ Core/Widgets/Concurrent/Test、ZzFluentUI、Windows MSVC。

- [x] 构建基础：锁定依赖版本，配置 C++20、Windows 默认限制、显式本地验证开关、工具资源。验证 CMake 配置及依赖接入，单独提交。
- [x] 解析和哈希：先写标准摘要、跨块文件、空文件、读取错误、取消、转义和本地化解析测试，运行红灯后实现；CTest 通过后提交。
- [x] 异步分析：以可执行测试工具模拟 aapt 边界，验证参数、失败、超时、取消、快速换文件和析构；实现任务管理并提交。
- [x] 纯代码界面：集成 ZzFluentStyle/ZzPushButton，表单、输出、进度、显式工具切换和拖放。验证打开、替换、无效拖放、取消及重试，提交。
- [x] Windows 交付：补齐构建预设、版本图标、部署、CI、中文说明和验收清单。运行完整本地构建/CTest/启动 smoke，独立审查并修正，分类提交。

每一步完成即记录验证结果；当前目录是用户指定的空仓库，直接在 codex/rewrite-windows 分支实现，不另建 worktree。

## 执行结果

- 所有实现任务已完成，按文档、构建、业务、界面、兼容修复、交付分类提交。
- 本地 5/5 CTest 套件通过，包含真实 Linux Android SDK 工具测试；窗口在 offscreen 和 Xvfb 下启动成功。
- 独立审查发现并修复了真实 aapt 引号格式、前导连字符参数以及 MSVC 便携运行库部署问题。
- Windows 图标/版本 RC 已经 MinGW windres 编译；Windows MSVC 构建、部署与实机交互需执行已提供的 CI 和验收清单。
