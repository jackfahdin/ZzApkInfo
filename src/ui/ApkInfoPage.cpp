#include "ApkInfoPage.h"
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QShortcut>
#include <QUrl>
#include <QVBoxLayout>
#include <ZzFluentUI/ZzPushButton.h>

namespace zzapk {
ApkInfoPage::ApkInfoPage(const QString &toolsDirectory, QWidget *parent)
    : QWidget(parent), analyzer_(toolsDirectory)
{
    buildUi();
    connect(&analyzer_, &ApkAnalyzer::started, this, [this](const QString &path) {
        currentPath_ = path;
        path_->setText(QDir::toNativeSeparators(path));
        path_->setCursorPosition(0);
        for (auto *field : fields_) field->clear();
        output_->clear();
        progress_->setValue(0);
    });
    connect(&analyzer_, &ApkAnalyzer::progressChanged, progress_, &QProgressBar::setValue);
    connect(&analyzer_, &ApkAnalyzer::finished, this, &ApkInfoPage::showResult);
    connect(&analyzer_, &ApkAnalyzer::rejected, output_, &QPlainTextEdit::appendPlainText);
}

void ApkInfoPage::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(8);
    auto *form = new QGridLayout;
    form->setHorizontalSpacing(6);
    form->setVerticalSpacing(6);
    form->setColumnStretch(1, 1);
    const auto makeField = [this](const QString &name) {
        auto *edit = new QLineEdit(this);
        edit->setObjectName(name);
        edit->setReadOnly(true);
        auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setPointSize(11);
        edit->setFont(font);
        return edit;
    };
    path_ = makeField(QStringLiteral("filePath"));
    path_->setAccessibleName(tr("文件路径"));
    form->addWidget(new QLabel(tr("文件 ："), this), 0, 0);
    form->addWidget(path_, 0, 1);
    select_ = new ZzFluentUI::ZzPushButton(tr("选择·aapt"), this);
    select_->setObjectName(QStringLiteral("selectButton"));
    select_->setToolTip(tr("右键点击切换到aapt2"));
    form->addWidget(select_, 0, 2);
    const QStringList labels{tr("名称 ："), tr("包名 ："), tr("类名 ："), tr("版本 ："), QStringLiteral("MD5 :")};
    const QStringList names{QStringLiteral("appName"), QStringLiteral("packageName"),
        QStringLiteral("activity"), QStringLiteral("versionName"), QStringLiteral("md5")};
    for (size_t i = 0; i < fields_.size(); ++i) {
        fields_[i] = makeField(names[qsizetype(i)]);
        fields_[i]->setAccessibleName(labels[qsizetype(i)]);
        form->addWidget(new QLabel(labels[qsizetype(i)], this), int(i) + 1, 0);
        form->addWidget(fields_[i], int(i) + 1, 1, 1, 2);
    }
    progress_ = new QProgressBar(this);
    progress_->setRange(0, 100);
    progress_->setValue(0);
    progress_->setAlignment(Qt::AlignCenter);
    progress_->setAccessibleName(tr("MD5 计算进度"));
    form->addWidget(new QLabel(tr("进度 ："), this), 6, 0);
    form->addWidget(progress_, 6, 1, 1, 2);
    layout->addLayout(form);
    auto *hint = new QLabel(tr("将apk拖入窗口或选择按钮处选择apk包"), this);
    hint->setObjectName(QStringLiteral("hint"));
    hint->setAlignment(Qt::AlignCenter);
    layout->addWidget(hint);
    output_ = new QPlainTextEdit(this);
    output_->setObjectName(QStringLiteral("output"));
    output_->setReadOnly(true);
    layout->addWidget(output_, 1);

    auto selectFile = [this] {
        const auto path = QFileDialog::getOpenFileName(this, tr("选择 APK 文件"),
            currentPath_.isEmpty() ? QString() : QFileInfo(currentPath_).absolutePath(), tr("APK 文件 (*.apk *.APK)"));
        if (!path.isEmpty()) openFile(path);
    };
    connect(select_, &QPushButton::clicked, this, selectFile);
    auto *shortcut = new QShortcut(QKeySequence::Open, this);
    connect(shortcut, &QShortcut::activated, this, selectFile);

    // 文本视口也统一接收 APK 拖放，防止编辑器把文件路径当作文本插入。
    installEventFilter(this);
    setAcceptDrops(true);
    for (auto *widget : findChildren<QWidget *>()) {
        widget->setAcceptDrops(true);
        widget->installEventFilter(this);
    }
}

void ApkInfoPage::switchTool()
{
    tool_ = tool_ == ApkTool::Aapt ? ApkTool::Aapt2 : ApkTool::Aapt;
    select_->setText(tool_ == ApkTool::Aapt ? tr("选择·aapt") : tr("选择·aapt2"));
    select_->setToolTip(tool_ == ApkTool::Aapt ? tr("右键点击切换到aapt2") : tr("右键点击切换到aapt"));
    if (!currentPath_.isEmpty()) openFile(currentPath_);
}

bool ApkInfoPage::openFile(const QString &path)
{
    return analyzer_.analyze(path, tool_);
}

void ApkInfoPage::showResult(const AnalysisResult &result)
{
    if (result.cancelled) return;
    const auto &metadata = result.metadata;
    const std::array<QString, 5> values{metadata.label, metadata.packageName, metadata.activity,
                                      metadata.versionName, result.md5};
    for (size_t i = 0; i < fields_.size(); ++i) {
        fields_[i]->setText(values[i]);
        fields_[i]->setCursorPosition(0);
    }
    output_->setPlainText(result.output);
    if (!result.diagnostics.isEmpty()) output_->appendPlainText(result.diagnostics);
    if (!result.error.isEmpty()) output_->appendPlainText(result.error);
}

QString ApkInfoPage::droppedApk(const QMimeData *mime) const
{
    if (!mime || !mime->hasUrls() || mime->urls().size() != 1) return {};
    const auto url = mime->urls().first();
    if (!url.isLocalFile()) return {};
    const QFileInfo info(url.toLocalFile());
    if (!info.isFile() || !info.isReadable()
        || info.suffix().compare(QStringLiteral("apk"), Qt::CaseInsensitive) != 0) return {};
    return info.absoluteFilePath();
}

bool ApkInfoPage::eventFilter(QObject *watched, QEvent *event)
{
    // ZzPushButton 为 final，使用事件过滤器保留右键切换工具的操作。
    if (watched == select_ && event->type() == QEvent::MouseButtonPress
        && static_cast<QMouseEvent *>(event)->button() == Qt::RightButton) {
        switchTool();
        return true;
    }
    if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove || event->type() == QEvent::Drop) {
        auto *drop = static_cast<QDropEvent *>(event);
        const auto path = droppedApk(drop->mimeData());
        if (!path.isEmpty()) {
            if (event->type() != QEvent::Drop || openFile(path)) drop->acceptProposedAction();
        } else {
            drop->ignore();
        }
        return true;
    }
    return QWidget::eventFilter(watched, event);
}
}
