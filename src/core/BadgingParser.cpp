#include "BadgingParser.h"
#include <QMap>
#include <QRegularExpression>

namespace zzapk {
namespace {
QString unescape(const QString &value)
{
    QString result;
    result.reserve(value.size());
    for (qsizetype i = 0; i < value.size(); ++i) {
        if (value[i] == u'\\' && i + 1 < value.size()
            && (value[i + 1] == u'\\' || value[i + 1] == u'\'')) {
            ++i;
        }
        result += value[i];
    }
    return result;
}
QString attribute(const QString &line, const QString &name)
{
    const QRegularExpression expression(
        QStringLiteral(R"((?:^|\s)%1='((?:\\.|[^'\\])*)')")
            .arg(QRegularExpression::escape(name)));
    return unescape(expression.match(line).captured(1));
}
}

ParseResult parseBadging(const QString &output)
{
    ParseResult result;
    QMap<QString, QString> labels;
    QString applicationLabel;
    for (const auto &rawLine : output.split(u'\n')) {
        const auto line = rawLine.trimmed();
        if (line.startsWith(QStringLiteral("package:"))) {
            result.metadata.packageName = attribute(line, QStringLiteral("name"));
            result.metadata.versionName = attribute(line, QStringLiteral("versionName"));
            result.metadata.versionCode = attribute(line, QStringLiteral("versionCode"));
        } else if (line.startsWith(QStringLiteral("launchable-activity:"))) {
            if (result.metadata.activity.isEmpty())
                result.metadata.activity = attribute(line, QStringLiteral("name"));
        } else if (line.startsWith(QStringLiteral("application-label"))) {
            const auto colon = line.indexOf(u':');
            if (colon < 0) continue;
            const auto value = line.mid(colon + 1).trimmed();
            if (value.size() >= 2 && value.startsWith(u'\'') && value.endsWith(u'\''))
                labels.insert(line.left(colon), unescape(value.mid(1, value.size() - 2)));
        } else if (line.startsWith(QStringLiteral("application:"))) {
            applicationLabel = attribute(line, QStringLiteral("label"));
        }
    }
    // 优先默认名称；缺失时采用中文、英文，最后回退到工具提供的其他语言。
    for (const auto &key : {"application-label", "application-label-zh-CN",
                            "application-label-zh", "application-label-en"}) {
        if (!labels.value(QLatin1String(key)).isEmpty()) {
            result.metadata.label = labels.value(QLatin1String(key));
            break;
        }
    }
    if (result.metadata.label.isEmpty()) result.metadata.label = applicationLabel;
    if (result.metadata.label.isEmpty()) {
        for (const auto &label : labels) {
            if (!label.isEmpty()) { result.metadata.label = label; break; }
        }
    }
    if (result.metadata.packageName.isEmpty())
        result.error = QStringLiteral("工具未返回有效的 APK 包信息，请检查文件或切换解析工具。");
    return result;
}
}
