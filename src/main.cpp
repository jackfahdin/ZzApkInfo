#include "ui/ApplicationSetup.h"
#include "ui/ApkInfoPage.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QIcon>
#include <QTimer>
#include <QDebug>
#include <ZzFluentUI/ZzFluentTitleBar.h>
#include <ZzPureTools/ZzApplicationWindow.h>
#include <ZzPureTools/ZzPureApplication.h>
#include <ZzWindowKit/ZzWindowKitBootstrap.h>

int main(int argc, char *argv[])
{
    if (!ZzWindowKit::ZzWindowKitBootstrap::prepare()) return 1;
    ZzPureTools::ZzPureApplication app(argc, argv);
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

    const auto built = zzapk::buildApplication(app, QCoreApplication::applicationDirPath(),
        parser.positionalArguments().value(0));
    if (!built) {
        qCritical().noquote() << built.error().technicalMessage();
        return 1;
    }
    if (parser.isSet(QStringLiteral("smoke"))) {
        QTimer::singleShot(300, &app, [&app] {
            const auto directory = QCoreApplication::applicationDirPath();
            bool windowReady = false;
            for (auto *widget : QApplication::topLevelWidgets()) {
                auto *window = qobject_cast<ZzPureTools::ZzApplicationWindow *>(widget);
                if (window && window->isVisible() && !window->windowIcon().pixmap(32, 32).isNull()
                    && window->titleBar()->isVisible() && window->windowAgent()
                    && window->findChild<zzapk::ApkInfoPage *>()) windowReady = true;
            }
            const bool ready = windowReady
                && QFileInfo::exists(directory + QStringLiteral("/aapt.exe"))
                && QFileInfo::exists(directory + QStringLiteral("/aapt2.exe"));
            app.exit(ready ? 0 : 1);
        });
    }
    const auto result = app.exec();
    app.beginShutdown();
    return result;
}
