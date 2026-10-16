#include "FileStager.h"
#include <QFile>

namespace zzapk {
StagedApk stageApk(const QString &path, std::stop_token stop)
{
    if (stop.stop_requested()) return {{}, {}, {}, true};
    StagedApk result;
    result.directory = std::make_shared<QTemporaryDir>();
    if (!result.directory->isValid()) {
        result.error = QStringLiteral("无法创建 aapt 兼容目录：%1").arg(result.directory->errorString());
        return result;
    }
    result.path = result.directory->filePath(QStringLiteral("input.apk"));
    QFile source(path);
    QFile destination(result.path);
    if (!source.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("无法读取 APK：%1").arg(source.errorString());
        return result;
    }
    if (!destination.open(QIODevice::WriteOnly)) {
        result.error = QStringLiteral("无法创建 aapt 临时文件：%1").arg(destination.errorString());
        return result;
    }
    QByteArray buffer(256 * 1024, Qt::Uninitialized);
    while (!stop.stop_requested()) {
        const auto count = source.read(buffer.data(), buffer.size());
        if (count < 0) { result.error = source.errorString(); return result; }
        if (count == 0) {
            if (!destination.flush()) result.error = destination.errorString();
            return result;
        }
        if (destination.write(buffer.constData(), count) != count) {
            result.error = QStringLiteral("写入 aapt 临时文件失败：%1").arg(destination.errorString());
            return result;
        }
    }
    result.cancelled = true;
    return result;
}
}
