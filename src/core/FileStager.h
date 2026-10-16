#pragma once
#include <QString>
#include <QTemporaryDir>
#include <memory>
#include <stop_token>

namespace zzapk {
struct StagedApk {
    std::shared_ptr<QTemporaryDir> directory;
    QString path;
    QString error;
    bool cancelled = false;
};
/** @brief 为旧 aapt 准备 ASCII 文件名；目录生命周期必须覆盖子进程。 */
[[nodiscard]] StagedApk stageApk(const QString &path, std::stop_token stop);
}
