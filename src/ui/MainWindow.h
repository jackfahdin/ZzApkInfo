#pragma once
#include "core/ApkAnalyzer.h"
#include <QWidget>
#include <array>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QMimeData;
namespace zzapk {
/** @brief 纯代码构建的 APK 信息窗口，负责交互和展示，不承担解析工作。 */
class MainWindow final : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(const QString &toolsDirectory, QWidget *parent = nullptr);
    bool openFile(const QString &path);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void buildUi();
    void showResult(const AnalysisResult &result);
    [[nodiscard]] QString droppedApk(const QMimeData *mime) const;
    ApkAnalyzer analyzer_;
    QString currentPath_;
    QComboBox *tool_ = nullptr;
    QLineEdit *path_ = nullptr;
    std::array<QLineEdit *, 6> fields_{};
    QPlainTextEdit *output_ = nullptr;
    QPlainTextEdit *diagnostics_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QLabel *status_ = nullptr;
    QPushButton *retry_ = nullptr;
    QPushButton *cancel_ = nullptr;
};
}
