#pragma once
#include <QString>

namespace zzapk {
/** @brief APK 的基本元数据；没有启动入口时 activity 保持为空。 */
struct ApkMetadata {
    QString label;
    QString packageName;
    QString activity;
    QString versionName;
    QString versionCode;
};
struct ParseResult {
    ApkMetadata metadata;
    QString error;
};
/** @brief 按 badging 记录类型解析，避免不同记录的同名属性相互覆盖。 */
[[nodiscard]] ParseResult parseBadging(const QString &output);
}
