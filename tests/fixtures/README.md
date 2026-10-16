# 最小 APK 测试样本

`sample.apk` 是本仓库自有的测试素材，仅含二进制 manifest 和字符串资源，无代码、无签名，不用于安装。原始文件位于同目录。

生成命令（Android SDK Build Tools 33.0.1 / Android 34）：

```sh
aapt package -f -M AndroidManifest.xml -S res -I /path/to/android-34/android.jar -F sample.apk
```

生成的 ZIP 时间戳会影响摘要。当前提交样本的 MD5 为 `5BE15C255287D26F28142D46098415AD`，SHA-256 为 `e57c69d3389a73730ef2fe0ed24b5609a94c7e3726211669eea3ebd4fddbaf63`。重新生成后需独立计算并更新测试中的摘要。

Windows 测试使用随项目交付的真实 aapt/aapt2，检查中文空格路径、元数据和标准文件摘要。Linux 开发检查可显式指定本机 SDK 工具目录（提供名为 `aapt.exe` / `aapt2.exe` 的符号链接），不把它们加入交付包。
