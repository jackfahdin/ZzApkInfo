# ZzApkInfo 重写设计

保留原有布局和功能，使用 ZzPureTools 的窗口及控件。产品仅支持 Windows，采用 C++20、Qt 6.8+ 与锁定版本的 ZzPureTools，不使用 Designer UI 文件。

## 功能与交互

保持 APK 文件选择、窗口拖放、aapt/aapt2 切换、应用名称、包名、启动 Activity、版本名称、MD5、进度和完整 badging 输出。文件行右侧保留「选择·aapt」按钮：左键选择文件、右键切换工具并重新解析。窗口默认置顶，初始 500×731，最小 500×700。保留六行字段、进度条、居中提示和单一输出区。字段和输出只读、可复制。

## 架构选择

使用 ZzPureApplication 和 ZzApplicationBuilder 装配 ZzApplicationWindow，提供框架无边框窗口、标题栏和窗口命令。隐藏单页工具不需要的导航面板，页面内使用 ZzPushButton 及 Fluent 样式。

- `src/core/BadgingParser`：按记录解析 aapt 文本，支持转义引号、默认/本地化名称及无启动入口 APK。
- `src/core/FileHasher`：后台流式计算标准文件 MD5，处理读取失败和取消；内存不随文件大小增长。
- `src/core/ApkAnalyzer`：异步 QProcess 与专用哈希任务，统一校验输入、状态、取消、超时、退出码和 stderr。新请求取消旧请求，序号隔离过期结果。
- `src/ui/ApkInfoPage`：字段、进度、输出和拖放；关闭时安全停止任务。
- `src/ui/ApplicationSetup`：装配应用及框架主窗口，配置单页布局和默认置顶。
- CMake/Windows CI：工具复制、Qt 部署、测试和启动检查，不写入源码目录。

## 错误修正

原始 MD5 对分块摘要二次哈希，结果不符合标准；新实现对原始字节顺序哈希。原代码 GUI 线程等待计算，新实现后台执行。工具路径以应用目录定位而非工作目录；记录 stderr、非零退出和超时；无效输入不启动任务；旧任务不能覆盖新文件。用 Qt 父子所有权与任务取消回收替代泄漏的计算对象。

## 验证边界

解析、哈希、任务状态与拖放使用 Qt Test 回归测试。当前主机是 Linux，可编译并运行同一业务/界面代码进行开发验证，但不发布 Linux 产品。默认非 Windows 配置拒绝构建，显式开发验证选项仅用于测试。Windows MSVC CI 执行构建、CTest、部署与启动 smoke；本地结果不冒充 Windows 实机验收。
