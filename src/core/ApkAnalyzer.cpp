#include "ApkAnalyzer.h"
#include "FileHasher.h"
#include "FileStager.h"
#include <algorithm>
#include <utility>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QProcess>
#include <QtConcurrentRun>

namespace zzapk {
namespace {
struct FileWorkResult {
    HashResult hash;
    StagedApk staged;
};
}
ApkAnalyzer::ApkAnalyzer(QString toolsDirectory, QObject *parent, int timeoutMs)
    : QObject(parent), toolsDirectory_(std::move(toolsDirectory)), timeoutMs_(timeoutMs)
{
    pool_.setMaxThreadCount(2);
    timeout_.setSingleShot(true);
    connect(&timeout_, &QTimer::timeout, this, [this] {
        if (!process_ || processDone_) return;
        result_.error = QStringLiteral("解析超时，已停止工具。请重试或切换解析工具。");
        process_->kill();
    });
}

ApkAnalyzer::~ApkAnalyzer()
{
    disconnect(this, nullptr, nullptr, nullptr);
    cancel();
    // 工作线程可能正在向本对象投递进度；等待结束后才能销毁 QObject。
    pool_.waitForDone();
    for (auto *process : findChildren<QProcess *>()) {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            process->waitForFinished(1000);
        }
    }
}

bool ApkAnalyzer::analyze(const QString &path, ApkTool tool)
{
    const QFileInfo info(path);
    if (!info.isFile() || info.suffix().compare(QStringLiteral("apk"), Qt::CaseInsensitive) != 0
        || !info.isReadable()) {
        emit rejected(QStringLiteral("请选择一个可读取的本地 APK 文件。"));
        return false;
    }
    cancel();
    const auto generation = ++generation_;
    stop_ = std::stop_source{};
    busy_ = true;
    processDone_ = false;
    hashDone_ = false;
    result_ = {};
    stdout_.clear(); stderr_.clear();
    result_.filePath = info.absoluteFilePath();
    modified_ = info.lastModified();
    size_ = info.size();
    emit busyChanged(true);
    emit started(result_.filePath);

    // 旧 Windows aapt 使用窄字符 argv。父目录通过 UTF-16 工作目录传入，
    // 非 ASCII 文件名则在后台复制为 input.apk；aapt2 原生支持 Unicode。
    bool stageInput = false;
#ifdef Q_OS_WIN
    const auto fileName = info.fileName();
    stageInput = tool == ApkTool::Aapt && std::any_of(fileName.begin(), fileName.end(),
        [](QChar character) { return character.unicode() > 127; });
#endif
    auto *watcher = new QFutureWatcher<FileWorkResult>(this);
    connect(watcher, &QFutureWatcher<FileWorkResult>::finished, this, [this, watcher, generation, stageInput] {
        const auto work = watcher->result();
        const auto &hash = work.hash;
        watcher->deleteLater();
        if (generation != generation_ || !busy_) return;
        hashDone_ = true;
        result_.md5 = hash.md5;
        if (!hash.error.isEmpty()) {
            if (!result_.error.isEmpty()) result_.error += u'\n';
            result_.error += hash.error;
        }
        if (stageInput) {
            if (!hash.error.isEmpty() || hash.cancelled) {
                processDone_ = true;
            } else {
                // 进程被取消后仍可能持有文件；由进程的销毁连接保活临时目录。
                connect(process_, &QObject::destroyed, [directory = work.staged.directory] {});
                startProcess(work.staged.path);
            }
        }
        finishIfReady();
    });
    watcher->setFuture(QtConcurrent::run(&pool_, [this, generation, path = result_.filePath,
                                                 token = stop_.get_token(), stageInput] {
        FileWorkResult work;
        if (stageInput) {
            work.staged = stageApk(path, token);
            if (work.staged.cancelled || !work.staged.error.isEmpty()) {
                work.hash = {{}, work.staged.error, work.staged.cancelled};
                return work;
            }
        }
        work.hash = hashFile(stageInput ? work.staged.path : path, token, [this, generation](int percent) {
            QMetaObject::invokeMethod(this, [this, generation, percent] {
                if (busy_ && generation == generation_) emit progressChanged(percent);
            }, Qt::QueuedConnection);
        });
        return work;
    }));

    auto *process = new QProcess(this);
    process_ = process;
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process] { drainOutput(process); });
    connect(process, &QProcess::readyReadStandardError, this, [this, process] { drainOutput(process); });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (process != process_) return;
        if (result_.error.isEmpty())
            result_.error = QStringLiteral("解析工具执行失败：%1\n工具位置：%2")
                .arg(process->errorString(), process->program());
        if (error == QProcess::FailedToStart) {
            processDone_ = true;
            timeout_.stop();
            finishIfReady();
        }
    });
    connect(process, &QProcess::finished, this, [this, process](int code, QProcess::ExitStatus status) {
        if (process != process_) return;
        timeout_.stop();
        drainOutput(process);
        processDone_ = true;
        if (result_.error.isEmpty()) {
            if (status != QProcess::NormalExit || code != 0) {
                result_.error = QStringLiteral("解析工具退出失败（退出码 %1），请查看下方输出。").arg(code);
            } else {
                const auto parsed = parseBadging(QString::fromUtf8(stdout_));
                result_.metadata = parsed.metadata;
                result_.error = parsed.error;
            }
        }
        finishIfReady();
    });
    const auto executable = tool == ApkTool::Aapt ? QStringLiteral("aapt.exe") : QStringLiteral("aapt2.exe");
    process->setProgram(QDir(toolsDirectory_).absoluteFilePath(executable));
    if (!stageInput) startProcess(result_.filePath);
    return true;
}

void ApkAnalyzer::startProcess(const QString &input)
{
    const QFileInfo info(input);
    process_->setWorkingDirectory(info.absolutePath());
    // 参数列表不经过 shell；相对文件名绕过旧工具对父目录编码的处理。
    process_->setArguments({QStringLiteral("dump"), QStringLiteral("badging"), QStringLiteral("./") + info.fileName()});
    timeout_.start(timeoutMs_);
    process_->start();
}

void ApkAnalyzer::drainOutput(QProcess *process)
{
    if (process != process_) return;
    constexpr qsizetype outputLimit = 16 * 1024 * 1024;
    const auto output = process->readAllStandardOutput();
    const auto diagnostic = process->readAllStandardError();
    const bool exceedsLimit = output.size() > outputLimit - stdout_.size()
        || diagnostic.size() > outputLimit - stderr_.size();
    stdout_ += output.left(outputLimit - stdout_.size());
    stderr_ += diagnostic.left(outputLimit - stderr_.size());
    if (exceedsLimit) {
        result_.error = QStringLiteral("工具输出超过 16 MiB 限制，已停止解析。");
        process->kill();
    }
}

void ApkAnalyzer::finishIfReady()
{
    if (!busy_ || !processDone_ || !hashDone_) return;
    timeout_.stop();
    result_.output = QString::fromUtf8(stdout_);
    result_.diagnostics = QString::fromUtf8(stderr_);
    const QFileInfo current(result_.filePath);
    if (current.size() != size_ || current.lastModified() != modified_) {
        result_.error += QStringLiteral("\n文件在分析期间发生变化，请重新解析。");
        result_.md5.clear();
        result_.metadata = {};
    }
    busy_ = false;
    stopProcess();
    emit busyChanged(false);
    emit finished(result_);
}

void ApkAnalyzer::stopProcess()
{
    if (!process_) return;
    auto *process = std::exchange(process_, nullptr);
    process->disconnect(this);
    if (process->state() == QProcess::NotRunning) {
        process->deleteLater();
    } else {
        connect(process, &QProcess::finished, process, &QObject::deleteLater);
        process->kill();
    }
}

void ApkAnalyzer::cancel()
{
    if (!busy_) return;
    ++generation_;
    stop_.request_stop();
    timeout_.stop();
    stopProcess();
    busy_ = false;
    AnalysisResult cancelled;
    cancelled.filePath = result_.filePath;
    cancelled.cancelled = true;
    emit busyChanged(false);
    emit finished(cancelled);
}
}
