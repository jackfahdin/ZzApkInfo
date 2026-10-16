#include "ui/MainWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QIcon>
#include <QTimer>
#include <ZzFluentUI/ZzFluentStyle.h>
#include <ZzFluentUI/ZzThemeController.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ZzApkInfo"));
    app.setOrganizationName(QStringLiteral("Jackfahdin"));
    app.setApplicationVersion(QStringLiteral(ZZAPK_VERSION));
    app.setWindowIcon(QIcon(QStringLiteral(":/app.ico")));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("APK 信息查看器"));
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({QStringLiteral("smoke"), QStringLiteral("执行窗口启动检查并退出")});
    parser.addPositionalArgument(QStringLiteral("apk"), QStringLiteral("可选的 APK 文件路径"));
    parser.process(app);
    if (parser.positionalArguments().size() > 1) parser.showHelp(2);

    // 控制器由应用持有，在 QApplication 销毁样式之后才释放。
    auto *theme = new ZzFluentUI::ZzThemeController(&app);
    app.setStyle(new ZzFluentUI::ZzFluentStyle(theme));
    zzapk::MainWindow window(QCoreApplication::applicationDirPath());
    window.show();
    if (!parser.positionalArguments().isEmpty()) window.openFile(parser.positionalArguments().first());
    if (parser.isSet(QStringLiteral("smoke"))) {
        QTimer::singleShot(300, &app, [&app, &window] {
            const auto directory = QCoreApplication::applicationDirPath();
            const bool ready = window.isVisible() && !window.windowIcon().isNull()
                && QFileInfo::exists(directory + QStringLiteral("/aapt.exe"))
                && QFileInfo::exists(directory + QStringLiteral("/aapt2.exe"));
            app.exit(ready ? 0 : 1);
        });
    }
    return app.exec();
}
