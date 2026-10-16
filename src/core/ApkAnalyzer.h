#pragma once
#include "BadgingParser.h"
#include <QDateTime>
#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include <stop_token>

class QProcess;
namespace zzapk {
enum class ApkTool { Aapt, Aapt2 };
struct AnalysisResult {
    QString filePath;
    ApkMetadata metadata;
    QString md5;
    QString output;
    QString diagnostics;
    QString error;
    bool cancelled = false;
};
/** @brief 管理单个活动分析请求；新请求替换旧请求，所有信号在 GUI 线程发出。 */
class ApkAnalyzer final : public QObject {
    Q_OBJECT
public:
    explicit ApkAnalyzer(QString toolsDirectory, QObject *parent = nullptr, int timeoutMs = 30000);
    ~ApkAnalyzer() override;
    [[nodiscard]] bool isBusy() const { return busy_; }
    bool analyze(const QString &path, ApkTool tool);
    void cancel();
signals:
    void started(const QString &path);
    void progressChanged(int percent);
    void finished(const zzapk::AnalysisResult &result);
    void rejected(const QString &reason);
    void busyChanged(bool busy);
private:
    void drainOutput(QProcess *process);
    void finishIfReady();
    void stopProcess();
    void startProcess(const QString &input);
    QString toolsDirectory_;
    int timeoutMs_;
    QThreadPool pool_;
    QTimer timeout_;
    QProcess *process_ = nullptr;
    std::stop_source stop_;
    quint64 generation_ = 0;
    bool busy_ = false;
    bool processDone_ = false;
    bool hashDone_ = false;
    QDateTime modified_;
    qint64 size_ = 0;
    QByteArray stdout_;
    QByteArray stderr_;
    AnalysisResult result_;
};
}
Q_DECLARE_METATYPE(zzapk::AnalysisResult)
