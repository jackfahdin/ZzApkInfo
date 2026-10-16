#pragma once
#include <QString>
#include <functional>
#include <stop_token>

namespace zzapk {
struct HashResult {
    QString md5;
    QString error;
    bool cancelled = false;
};
/** @brief 顺序读取原始字节计算标准 MD5；回调发生在调用线程。 */
[[nodiscard]] HashResult hashFile(const QString &path, std::stop_token stop = {},
                                 const std::function<void(int)> &progress = {});
}
