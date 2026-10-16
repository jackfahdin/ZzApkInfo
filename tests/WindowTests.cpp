#include "ui/ApkInfoPage.h"
#include "ui/ApplicationSetup.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>
#include <ZzFluentUI/ZzFluentTitleBar.h>
#include <ZzFluentUI/ZzNavigationPane.h>
#include <ZzFluentUI/ZzPushButton.h>
#include <ZzPureTools/ZzApplicationWindow.h>
#include <ZzPureTools/ZzPureApplication.h>
#include <ZzWindowKit/ZzWindowKitBootstrap.h>

class WindowTests : public QObject {
    Q_OBJECT
    QTemporaryDir dir;
    QPointer<ZzPureTools::ZzApplicationWindow> window;
    zzapk::ApkInfoPage *page = nullptr;
    QString apk(const QString &name, const QByteArray &data = "abc") {
        QFile file(dir.filePath(name));
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()) return {};
        return file.fileName();
    }
private slots:
    void initTestCase() {
        auto &app = *static_cast<ZzPureTools::ZzPureApplication *>(qApp);
        const auto result = zzapk::buildApplication(app, QCoreApplication::applicationDirPath());
        QVERIFY2(result.hasValue(), result ? "" : qPrintable(result.error().technicalMessage()));
        for (auto *widget : QApplication::topLevelWidgets())
            if (qobject_cast<ZzPureTools::ZzApplicationWindow *>(widget)) widget->close();
        QTRY_COMPARE(app.windowCount(), 0);
    }
    void init() {
        const auto created = static_cast<ZzPureTools::ZzPureApplication *>(qApp)->createWindow();
        QVERIFY2(created.hasValue(), created ? "" : qPrintable(created.error().technicalMessage()));
        window = created.value();
        page = window->findChild<zzapk::ApkInfoPage *>();
        QVERIFY(page);
    }
    void cleanup() {
        window->close();
        QTRY_VERIFY(window.isNull());
        page = nullptr;
    }
    void originalLayoutAndChrome() {
        QVERIFY(window->titleBar());
        QVERIFY(window->windowAgent());
        QVERIFY(window->titleBar()->isVisible());
        QVERIFY(window->navigationPane()->isHidden());
        QVERIFY(window->windowFlags().testFlag(Qt::WindowStaysOnTopHint));
        QCOMPARE(window->size(), QSize(500, 731));
        QCOMPARE(window->minimumSize(), QSize(500, 700));
        QCOMPARE(page->findChildren<QLineEdit *>().size(), 6);
        QCOMPARE(page->findChildren<QPlainTextEdit *>().size(), 1);
        QCOMPARE(page->findChildren<QPushButton *>().size(), 1);
        const QStringList names{"filePath", "appName", "packageName", "activity", "versionName", "md5"};
        int lastY = -1;
        for (const auto &name : names) {
            auto *field = page->findChild<QLineEdit *>(name);
            QVERIFY(field && field->isReadOnly());
            const auto y = field->mapTo(page, QPoint{}).y();
            QVERIFY(y > lastY);
            lastY = y;
        }
        auto *select = page->findChild<ZzFluentUI::ZzPushButton *>("selectButton");
        QVERIFY(select);
        QCOMPARE(select->text(), QStringLiteral("选择·aapt"));
        auto *path = page->findChild<QLineEdit *>("filePath");
        QCOMPARE(select->mapTo(page, QPoint{}).y(), path->mapTo(page, QPoint{}).y());
        QVERIFY(select->mapTo(page, QPoint{}).x() > path->mapTo(page, QPoint{}).x());
        QCOMPARE(page->findChild<QLabel *>("hint")->text(), QStringLiteral("将apk拖入窗口或选择按钮处选择apk包"));
    }
    void openAndSwitch() {
        QVERIFY(page->openFile(apk("hello.APK")));
        auto *package = page->findChild<QLineEdit *>("packageName");
        auto *md5 = page->findChild<QLineEdit *>("md5");
        auto *output = page->findChild<QPlainTextEdit *>("output");
        auto *select = page->findChild<QPushButton *>("selectButton");
        QTRY_COMPARE(package->text(), "hello");
        QCOMPARE(md5->text(), "900150983CD24FB0D6963F7D28E17F72");
        QVERIFY(output->isReadOnly());
        QVERIFY(output->toPlainText().contains("tool warning (aapt.exe)"));
        QCOMPARE(page->findChild<QProgressBar *>()->value(), 100);
        QTest::mouseClick(select, Qt::RightButton);
        QCOMPARE(select->text(), QStringLiteral("选择·aapt2"));
        QCOMPARE(package->text(), QString());
        QTRY_COMPARE(package->text(), "hello");
        QVERIFY(output->toPlainText().contains("tool warning (aapt2.exe)"));
        QTest::mouseClick(select, Qt::RightButton);
        QCOMPARE(select->text(), QStringLiteral("选择·aapt"));
        QTRY_COMPARE(package->text(), "hello");
        if (const auto preview = qEnvironmentVariable("ZZAPK_PREVIEW_PATH"); !preview.isEmpty()) {
            QTest::qWait(150);
            QVERIFY(window->grab().save(preview));
        }
        QVERIFY(!page->openFile(dir.filePath("missing.apk")));
        QCOMPARE(package->text(), "hello");
        QVERIFY(!output->toPlainText().isEmpty());
    }
    void selectFile() {
        const auto path = apk("selected.apk");
        bool dialogSeen = false;
        QTimer::singleShot(100, page, [&] {
            auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
            dialogSeen = dialog != nullptr;
            if (dialog) {
                auto *fileName = dialog->findChild<QLineEdit *>("fileNameEdit");
                QVERIFY(fileName);
                fileName->setText(path);
                QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
            }
        });
        QTimer timeout;
        timeout.setSingleShot(true);
        connect(&timeout, &QTimer::timeout, page, [] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) dialog->reject();
        });
        timeout.start(3000);
        page->findChild<QPushButton *>("selectButton")->click();
        QVERIFY(dialogSeen);
        QTRY_COMPARE(page->findChild<QLineEdit *>("packageName")->text(), "selected");
    }
    void outputViewportDrops() {
        QTest::mouseClick(page->findChild<QPushButton *>("selectButton"), Qt::RightButton);
        auto *output = page->findChild<QPlainTextEdit *>("output");
        QMimeData mime; mime.setUrls({QUrl::fromLocalFile(apk(QStringLiteral("拖放 空格.apk")))});
        QDragEnterEvent enter({5, 5}, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(output->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        QDropEvent drop({5, 5}, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(output->viewport(), &drop);
        QVERIFY(drop.isAccepted());
        QTRY_COMPARE(page->findChild<QLineEdit *>("packageName")->text(), QStringLiteral("拖放 空格"));
        mime.setUrls({QUrl("https://example.com/file.apk")});
        QDragEnterEvent remote({5, 5}, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(output->viewport(), &remote); QVERIFY(!remote.isAccepted());
        mime.setUrls({QUrl::fromLocalFile(apk("a.apk")), QUrl::fromLocalFile(apk("b.apk"))});
        QDragEnterEvent multiple({5, 5}, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(output->viewport(), &multiple); QVERIFY(!multiple.isAccepted());
    }
    void replaceRunningAnalysisAndReportErrors() {
        QVERIFY(page->openFile(apk("slow.apk", "slow")));
        QVERIFY(page->openFile(apk("latest.apk")));
        auto *package = page->findChild<QLineEdit *>("packageName");
        QTRY_COMPARE(package->text(), "latest");
        QTest::qWait(2100);
        QCOMPARE(package->text(), "latest");
        QVERIFY(page->openFile(apk("invalid.apk", "error")));
        auto *output = page->findChild<QPlainTextEdit *>("output");
        QTRY_VERIFY(output->toPlainText().contains("invalid archive"));
        QVERIFY(package->text().isEmpty());
    }
};
int main(int argc, char **argv) {
    if (!ZzWindowKit::ZzWindowKitBootstrap::prepare()) return 1;
    ZzPureTools::ZzPureApplication app(argc, argv);
    app.setApplicationVersion(QStringLiteral(ZZAPK_VERSION));
    app.setQuitOnLastWindowClosed(false);
    app.setAttribute(Qt::AA_DontUseNativeDialogs);
    WindowTests tests;
    const auto result = QTest::qExec(&tests, argc, argv);
    app.beginShutdown();
    return result;
}
#include "WindowTests.moc"
