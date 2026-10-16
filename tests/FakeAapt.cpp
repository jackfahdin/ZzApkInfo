#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <cstdio>

// 只替代外部 Android 工具；进程启动、参数传递、退出和取消均使用真实 QProcess。
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 4 || args[1] != "dump" || args[2] != "badging") return 12;
    QFile file(args[3]);
    if (!file.open(QIODevice::ReadOnly)) return 13;
    const auto mode = file.readAll();
    if (mode == "slow") QThread::msleep(2000);
    if (mode == "error") { std::fputs("invalid archive", stderr); return 7; }
    if (mode == "malformed") { std::puts("not badging"); return 0; }
    if (mode == "flood") {
        const QByteArray block(1024 * 1024, 'x');
        for (int i = 0; i < 20; ++i) std::fwrite(block.data(), 1, size_t(block.size()), stdout);
        return 0;
    }
    const auto package = QFileInfo(args[3]).baseName().toUtf8();
    std::printf("package: name='%s' versionCode='1' versionName='1.0'\n", package.constData());
    std::puts("application-label:'Example'");
    std::fprintf(stderr, "tool warning (%s)", QFileInfo(app.applicationFilePath()).fileName().toUtf8().constData());
    return 0;
}
