#include "MainWindow.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QShortcut>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <ZzFluentUI/ZzPushButton.h>

namespace zzapk {
MainWindow::MainWindow(const QString &toolsDirectory, QWidget *parent)
    : QWidget(parent), analyzer_(toolsDirectory)
{
    buildUi();
    connect(&analyzer_, &ApkAnalyzer::started, this, [this](const QString &path) {
        currentPath_ = path;
        path_->setText(QDir::toNativeSeparators(path));
        path_->setCursorPosition(0);
        for (auto *field : fields_) field->clear();
        output_->clear(); diagnostics_->clear();
        progress_->setValue(0);
        status_->setText(tr("正在解析 APK 并计算 MD5…"));
    });
    connect(&analyzer_, &ApkAnalyzer::progressChanged, progress_, &QProgressBar::setValue);
    connect(&analyzer_, &ApkAnalyzer::finished, this, &MainWindow::showResult);
    connect(&analyzer_, &ApkAnalyzer::rejected, status_, &QLabel::setText);
    connect(&analyzer_, &ApkAnalyzer::busyChanged, this, [this](bool busy) {
        cancel_->setEnabled(busy);
        retry_->setEnabled(!busy && !currentPath_.isEmpty());
    });
    connect(tool_, &QComboBox::currentIndexChanged, this, [this] {
        if (!currentPath_.isEmpty()) openFile(currentPath_);
    });
    connect(retry_, &QPushButton::clicked, this, [this] { openFile(currentPath_); });
    connect(cancel_, &QPushButton::clicked, &analyzer_, &ApkAnalyzer::cancel);
}

void MainWindow::buildUi()
{
    setWindowTitle(tr("ZzApkInfo · APK 信息"));
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    resize(820, 780);
    setMinimumSize(660, 640);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto *heading = new QLabel(tr("APK 信息查看器"), this);
    auto titleFont = heading->font();
    titleFont.setPointSize(20); titleFont.setBold(true); heading->setFont(titleFont);
    layout->addWidget(heading);
    auto *hint = new QLabel(tr("拖入一个 APK 文件，或打开文件查看应用信息与 MD5。"), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *commands = new QHBoxLayout;
    auto *open = new ZzFluentUI::ZzPushButton(tr("打开 APK…"), this);
    open->setObjectName(QStringLiteral("open"));
    open->setAppearance(ZzFluentUI::ZzButtonAppearance::Accent);
    open->setToolTip(tr("打开 APK 文件（Ctrl+O）"));
    commands->addWidget(open);
    auto *toolLabel = new QLabel(tr("解析工具"), this);
    tool_ = new QComboBox(this);
    tool_->setObjectName(QStringLiteral("tool"));
    tool_->addItems({QStringLiteral("aapt"), QStringLiteral("aapt2")});
    tool_->setAccessibleName(tr("解析工具"));
    tool_->setToolTip(tr("切换后自动重新解析当前 APK"));
    toolLabel->setBuddy(tool_);
    commands->addSpacing(8);
    commands->addWidget(toolLabel); commands->addWidget(tool_);
    commands->addStretch();
    auto *topmost = new QCheckBox(tr("窗口置顶"), this);
    topmost->setChecked(true);
    connect(topmost, &QCheckBox::toggled, this, [this](bool checked) {
        setWindowFlag(Qt::WindowStaysOnTopHint, checked);
        show();
    });
    commands->addWidget(topmost);
    layout->addLayout(commands);

    auto selectFile = [this] {
        const auto path = QFileDialog::getOpenFileName(this, tr("选择 APK 文件"),
            currentPath_.isEmpty() ? QString() : QFileInfo(currentPath_).absolutePath(), tr("APK 文件 (*.apk *.APK)"));
        if (!path.isEmpty()) openFile(path);
    };
    connect(open, &QPushButton::clicked, this, selectFile);
    auto *openShortcut = new QShortcut(QKeySequence::Open, this);
    connect(openShortcut, &QShortcut::activated, this, selectFile);

    auto *details = new QGroupBox(tr("应用信息"), this);
    auto *form = new QFormLayout(details);
    form->setContentsMargins(16, 20, 16, 16);
    form->setSpacing(10);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    const auto makeField = [details](const QString &name) {
        auto *edit = new QLineEdit(details);
        edit->setObjectName(name);
        edit->setReadOnly(true);
        edit->setPlaceholderText(QStringLiteral("—"));
        return edit;
    };
    path_ = makeField(QStringLiteral("filePath"));
    path_->setAccessibleName(tr("文件路径"));
    form->addRow(tr("文件"), path_);
    const QStringList labels{tr("名称"), tr("包名"), tr("启动类"), tr("版本名称"), tr("版本代码"), QStringLiteral("MD5")};
    const QStringList names{QStringLiteral("appName"), QStringLiteral("packageName"), QStringLiteral("activity"),
        QStringLiteral("versionName"), QStringLiteral("versionCode"), QStringLiteral("md5")};
    for (size_t i = 0; i < fields_.size(); ++i) {
        fields_[i] = makeField(names[qsizetype(i)]);
        fields_[i]->setAccessibleName(labels[qsizetype(i)]);
        form->addRow(labels[qsizetype(i)], fields_[i]);
    }
    fields_[5]->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(details);

    auto *progressRow = new QHBoxLayout;
    progress_ = new QProgressBar(this);
    progress_->setRange(0, 100); progress_->setValue(0);
    progress_->setFormat(tr("MD5  %p%"));
    progress_->setAccessibleName(tr("MD5 计算进度"));
    progressRow->addWidget(progress_, 1);
    retry_ = new ZzFluentUI::ZzPushButton(tr("重新解析"), this);
    retry_->setObjectName(QStringLiteral("retry")); retry_->setEnabled(false);
    cancel_ = new ZzFluentUI::ZzPushButton(tr("取消"), this);
    cancel_->setObjectName(QStringLiteral("cancel")); cancel_->setEnabled(false);
    progressRow->addWidget(retry_); progressRow->addWidget(cancel_);
    layout->addLayout(progressRow);
    auto *cancelShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(cancelShortcut, &QShortcut::activated, &analyzer_, &ApkAnalyzer::cancel);

    status_ = new QLabel(tr("尚未选择 APK 文件"), this);
    status_->setObjectName(QStringLiteral("status"));
    status_->setTextFormat(Qt::PlainText);
    status_->setWordWrap(true);
    status_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(status_);

    auto *tabs = new QTabWidget(this);
    const auto makeOutput = [tabs](const QString &name) {
        auto *edit = new QPlainTextEdit(tabs);
        edit->setObjectName(name);
        edit->setReadOnly(true);
        edit->setLineWrapMode(QPlainTextEdit::NoWrap);
        edit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        return edit;
    };
    output_ = makeOutput(QStringLiteral("output"));
    output_->setPlaceholderText(tr("完整的 aapt / aapt2 badging 输出将显示在这里。"));
    diagnostics_ = makeOutput(QStringLiteral("diagnostics"));
    diagnostics_->setPlaceholderText(tr("工具警告与错误将显示在这里。"));
    tabs->addTab(output_, tr("原始输出"));
    tabs->addTab(diagnostics_, tr("工具诊断"));
    layout->addWidget(tabs, 1);

    // 统一处理所有子控件（包括文本视口）的文件拖放，避免文本编辑器插入路径。
    installEventFilter(this);
    setAcceptDrops(true);
    for (auto *widget : findChildren<QWidget *>()) {
        widget->setAcceptDrops(true);
        widget->installEventFilter(this);
    }
}

bool MainWindow::openFile(const QString &path)
{
    return analyzer_.analyze(path, tool_->currentIndex() == 0 ? ApkTool::Aapt : ApkTool::Aapt2);
}

void MainWindow::showResult(const AnalysisResult &result)
{
    if (result.cancelled) {
        status_->setText(tr("已取消。可以重新解析或打开其他 APK。"));
        return;
    }
    const auto &metadata = result.metadata;
    const std::array<QString, 6> values{metadata.label, metadata.packageName, metadata.activity,
                                      metadata.versionName, metadata.versionCode, result.md5};
    for (size_t i = 0; i < fields_.size(); ++i) {
        fields_[i]->setText(values[i]); fields_[i]->setCursorPosition(0);
    }
    fields_[2]->setPlaceholderText(result.error.isEmpty() ? tr("无启动入口") : QStringLiteral("—"));
    output_->setPlainText(result.output);
    diagnostics_->setPlainText(result.diagnostics);
    if (!result.error.isEmpty()) {
        diagnostics_->appendPlainText(result.error);
        status_->setText(tr("分析未完成：%1").arg(result.error));
    } else {
        status_->setText(result.diagnostics.isEmpty() ? tr("分析完成") : tr("分析完成，工具有提示，请查看「工具诊断」。"));
    }
}

QString MainWindow::droppedApk(const QMimeData *mime) const
{
    if (!mime || !mime->hasUrls() || mime->urls().size() != 1) return {};
    const auto url = mime->urls().first();
    if (!url.isLocalFile()) return {};
    const QFileInfo info(url.toLocalFile());
    if (!info.isFile() || !info.isReadable()
        || info.suffix().compare(QStringLiteral("apk"), Qt::CaseInsensitive) != 0) return {};
    return info.absoluteFilePath();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
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
