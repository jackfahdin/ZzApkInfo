#include "FileHasher.h"
#include <QCryptographicHash>
#include <QFile>
#include <algorithm>

namespace zzapk {
HashResult hashFile(const QString &path, std::stop_token stop,
                    const std::function<void(int)> &progress)
{
    if (stop.stop_requested()) return {{}, {}, true};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {{}, QStringLiteral("无法读取文件：%1").arg(file.errorString()), false};
    const auto expectedSize = file.size();
    QCryptographicHash hash(QCryptographicHash::Md5);
    QByteArray buffer(256 * 1024, Qt::Uninitialized);
    qint64 total = 0;
    int previousProgress = -1;
    if (progress) progress(0);
    while (true) {
        if (stop.stop_requested()) return {{}, {}, true};
        const auto count = file.read(buffer.data(), buffer.size());
        if (count < 0)
            return {{}, QStringLiteral("读取文件失败：%1").arg(file.errorString()), false};
        if (count == 0) break;
        hash.addData(QByteArrayView(buffer.constData(), count));
        total += count;
        // 浮点比例避免超大文件的整数乘法溢出；100% 只在读完并校验后报告。
        const int percent = expectedSize > 0
            ? std::clamp(static_cast<int>(double(total) / double(expectedSize) * 100), 0, 99) : 0;
        if (progress && percent != previousProgress) progress(percent);
        previousProgress = percent;
    }
    if (stop.stop_requested()) return {{}, {}, true};
    if (total != expectedSize || file.size() != expectedSize)
        return {{}, QStringLiteral("文件在计算期间发生变化，请重新解析。"), false};
    if (progress) progress(100);
    return {QString::fromLatin1(hash.result().toHex().toUpper()), {}, false};
}
}
