#include "ApplicationSetup.h"
#include "ApkInfoPage.h"
#include <QCoreApplication>
#include <QIcon>
#include <ZzFluentUI/ZzNavigationPane.h>
#include <ZzPureTools/ZzApplicationBuilder.h>
#include <ZzPureTools/ZzApplicationWindow.h>
#include <ZzPureTools/ZzPageInstance.h>
#include <ZzPureTools/ZzPureApplication.h>

namespace zzapk {
ZzCore::ZzResult<void> buildApplication(ZzPureTools::ZzPureApplication &application,
    const QString &toolsDirectory, const QString &initialPath)
{
    ZzPureTools::ZzApplicationBuilder builder;
    auto configured = builder.setWindowSetupCallback([](ZzPureTools::ZzApplicationWindow &window) {
        window.setWindowTitle(QStringLiteral("ZzApkInfo-v%1 (Jackfahdin)")
            .arg(QCoreApplication::applicationVersion()));
        window.setWindowIcon(QIcon(QStringLiteral(":/app.ico")));
        window.setWindowFlag(Qt::WindowStaysOnTopHint, true);
        window.resize(500, 731);
        window.setMinimumSize(500, 700);
        // 单页工具无需导航栏；保留框架标题栏和原有默认置顶行为。
        window.navigationPane()->hide();
        return ZzCore::ZzResult<void>::success();
    });
    if (!configured) return configured;
    const ZzPureTools::ZzRouteId route(QStringLiteral("apk-info"));
    ZzPureTools::ZzPageRegistration page;
    page.routeId = route;
    page.factory = [toolsDirectory, initialPath](QWidget *parent) {
        auto *view = new ApkInfoPage(toolsDirectory, parent);
        if (!initialPath.isEmpty()) view->openFile(initialPath);
        return ZzPureTools::ZzPageInstance::create(parent, view,
            std::make_unique<QObject>(), std::make_unique<QObject>());
    };
    configured = builder.addPage(std::move(page));
    if (!configured) return configured;
    configured = builder.setInitialRoute(route);
    return configured ? builder.build(application) : configured;
}
}
