#pragma once
#include "core/ApkAnalyzer.h"
#include <QWidget>
#include <array>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QMimeData;
namespace zzapk {
/** @brief APK 信息页面，负责交互和展示，不承担解析工作。 */
class ApkInfoPage final : public QWidget {
    Q_OBJECT
public:
    explicit ApkInfoPage(const QString &toolsDirectory, QWidget *parent = nullptr);
    bool openFile(const QString &path);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void buildUi();
    void showResult(const AnalysisResult &result);
    [[nodiscard]] QString droppedApk(const QMimeData *mime) const;
    ApkAnalyzer analyzer_;
    QString currentPath_;
    ApkTool tool_ = ApkTool::Aapt;
    QPushButton *select_ = nullptr;
    QLineEdit *path_ = nullptr;
    std::array<QLineEdit *, 5> fields_{};
    QPlainTextEdit *output_ = nullptr;
    QProgressBar *progress_ = nullptr;
    void switchTool();
};
}
