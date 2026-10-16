#include "ui/MainWindow.h"
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>
#include <ZzFluentUI/ZzFluentStyle.h>
#include <ZzFluentUI/ZzThemeController.h>

class WindowTests : public QObject {
    Q_OBJECT
    QTemporaryDir dir;
    QString apk(const QString &name, const QByteArray &data = "abc") {
        QFile file(dir.filePath(name));
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()) return {};
        return file.fileName();
    }
private slots:
    void openAndSwitch() {
        zzapk::MainWindow window(QCoreApplication::applicationDirPath());
        QVERIFY(window.openFile(apk("hello.APK")));
        auto *package = window.findChild<QLineEdit *>("packageName");
        auto *md5 = window.findChild<QLineEdit *>("md5");
        auto *output = window.findChild<QPlainTextEdit *>("output");
        auto *tool = window.findChild<QComboBox *>("tool");
        QVERIFY(package && md5 && output && tool);
        QTRY_COMPARE(package->text(), "hello");
        QCOMPARE(md5->text(), "900150983CD24FB0D6963F7D28E17F72");
        QVERIFY(package->isReadOnly()); QVERIFY(output->isReadOnly());
        tool->setCurrentIndex(1);
        QCOMPARE(package->text(), QString());
        QTRY_COMPARE(package->text(), "hello");
        if (const auto preview = qEnvironmentVariable("ZZAPK_PREVIEW_PATH"); !preview.isEmpty()) {
            window.show();
            QTest::qWait(150);
            QVERIFY(window.grab().save(preview));
        }
        QVERIFY(!window.openFile(dir.filePath("missing.apk")));
        QCOMPARE(package->text(), "hello");
    }
    void outputViewportDrops() {
        zzapk::MainWindow window(QCoreApplication::applicationDirPath());
        window.show();
        window.findChild<QComboBox *>("tool")->setCurrentIndex(1);
        auto *output = window.findChild<QPlainTextEdit *>("output");
        QVERIFY(output);
        QMimeData mime; mime.setUrls({QUrl::fromLocalFile(apk(QStringLiteral("拖放 空格.apk")))});
        QDragEnterEvent enter({5, 5}, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(output->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        QDropEvent drop({5, 5}, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(output->viewport(), &drop);
        QVERIFY(drop.isAccepted());
        auto *package = window.findChild<QLineEdit *>("packageName");
        QVERIFY(package); QTRY_COMPARE(package->text(), QStringLiteral("拖放 空格"));
        mime.setUrls({QUrl("https://example.com/file.apk")});
        QDragEnterEvent remote({5, 5}, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(output->viewport(), &remote); QVERIFY(!remote.isAccepted());
        mime.setUrls({QUrl::fromLocalFile(apk("a.apk")), QUrl::fromLocalFile(apk("b.apk"))});
        QDragEnterEvent multiple({5, 5}, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(output->viewport(), &multiple); QVERIFY(!multiple.isAccepted());
    }
    void cancelAndRetry() {
        zzapk::MainWindow window(QCoreApplication::applicationDirPath());
        QVERIFY(window.openFile(apk("slow.apk", "slow")));
        auto *cancel = window.findChild<QPushButton *>("cancel");
        auto *retry = window.findChild<QPushButton *>("retry");
        QVERIFY(cancel && retry); QVERIFY(cancel->isEnabled()); QVERIFY(!retry->isEnabled());
        cancel->click(); QVERIFY(!cancel->isEnabled()); QVERIFY(retry->isEnabled());
        apk("slow.apk", "abc"); retry->click();
        auto *package = window.findChild<QLineEdit *>("packageName");
        QVERIFY(package); QTRY_COMPARE(package->text(), "slow");
    }
};
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    // 样式先销毁，控制器后销毁，满足组件生命周期契约。
    auto *theme = new ZzFluentUI::ZzThemeController(&app);
    app.setStyle(new ZzFluentUI::ZzFluentStyle(theme));
    WindowTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "WindowTests.moc"
