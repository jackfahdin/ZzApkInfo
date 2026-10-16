#pragma once
#include <ZzCore/ZzResult.h>
#include <QString>

namespace ZzPureTools { class ZzPureApplication; }
namespace zzapk {
/** @brief 装配单页应用，窗口、标题栏及生命周期交由 ZzPureTools 管理。 */
ZzCore::ZzResult<void> buildApplication(ZzPureTools::ZzPureApplication &application,
    const QString &toolsDirectory, const QString &initialPath = {});
}
