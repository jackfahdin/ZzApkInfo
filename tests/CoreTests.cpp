#include "core/BadgingParser.h"
#include "core/FileHasher.h"
#include "core/FileStager.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class CoreTests : public QObject {
    Q_OBJECT
private slots:
    void standardMd5_data() {
        QTest::addColumn<QByteArray>("data");
        QTest::addColumn<QString>("digest");
        QTest::newRow("empty") << QByteArray() << QString("D41D8CD98F00B204E9800998ECF8427E");
        QTest::newRow("abc") << QByteArray("abc") << QString("900150983CD24FB0D6963F7D28E17F72");
        // 跨越读取块，捕获旧实现“分块摘要再哈希”的错误。
        QTest::newRow("million-a") << QByteArray(1000000, 'a') << QString("7707D6AE4E027C70EEA2A935C2296F21");
    }
    void standardMd5() {
        QFETCH(QByteArray, data); QFETCH(QString, digest);
        QTemporaryDir dir;
        QFile file(dir.filePath(QStringLiteral("中文 空格.APK")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(data), data.size()); file.close();
        QList<int> updates;
        const auto result = zzapk::hashFile(file.fileName(), {}, [&](int p) { updates << p; });
        QVERIFY(result.error.isEmpty()); QVERIFY(!result.cancelled);
        QCOMPARE(result.md5, digest);
        QVERIFY(!updates.isEmpty()); QCOMPARE(updates.last(), 100);
        QVERIFY(std::is_sorted(updates.begin(), updates.end()));
    }
    void readFailure() {
        const auto result = zzapk::hashFile("/missing/path/no.apk");
        QVERIFY(!result.error.isEmpty()); QVERIFY(result.md5.isEmpty());
    }
    void cancellation() {
        QTemporaryDir dir; QFile file(dir.filePath("large.apk"));
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write(QByteArray(2000000, 'a')); file.close();
        std::stop_source stop;
        const auto result = zzapk::hashFile(file.fileName(), stop.get_token(), [&](int p) {
            if (p > 0) stop.request_stop();
        });
        QVERIFY(result.cancelled); QVERIFY(result.md5.isEmpty());
    }
    void parseRecords() {
        const auto result = zzapk::parseBadging(QString::fromUtf8(
            "package: name='org.example' versionCode='42' versionName='1.2'\r\n"
            "application-label:'测试应用'\r\n"
            "launchable-activity: name='org.example.Main' label='' icon=''\r\n"
            "uses-permission: name='android.permission.INTERNET'\n"));
        QVERIFY(result.error.isEmpty());
        QCOMPARE(result.metadata.packageName, "org.example");
        QCOMPARE(result.metadata.label, QStringLiteral("测试应用"));
        QCOMPARE(result.metadata.activity, "org.example.Main");
        QCOMPARE(result.metadata.versionName, "1.2");
        QCOMPARE(result.metadata.versionCode, "42");
    }
    void fallbackAndEscapes() {
        const auto result = zzapk::parseBadging(
            "package: versionName='it\\'s 2' name='org.test' versionCode='2'\n"
            "application-label-fr:'Bonjour'\napplication-label-zh-CN:'中文'\n");
        QVERIFY(result.error.isEmpty());
        QCOMPARE(result.metadata.label, QStringLiteral("中文"));
        QCOMPARE(result.metadata.versionName, "it's 2");
        QVERIFY(result.metadata.activity.isEmpty());
        const auto defaults = zzapk::parseBadging(
            "package: name='org.test'\napplication-label-zh-CN:'中文'\napplication-label:'Default'\n");
        QCOMPARE(defaults.metadata.label, "Default");
        QCOMPARE(zzapk::parseBadging("package: name='org.test'\napplication: label='App' icon=''\n").metadata.label, "App");
    }
    void unescapedApostropheAndQuotedLabel() {
        // 真实 aapt 保留单引号，仅对双引号和反斜线等字符转义。
        const auto result = zzapk::parseBadging(QString::fromUtf8(
            R"(package: name='org.test' versionCode='7' versionName='it's \"2\"' platformBuildVersionName='14')"
            "\n"
            R"(application-label:'测试 \"双引号\"')"));
        QCOMPARE(result.metadata.versionName, QStringLiteral("it's \"2\""));
        QCOMPARE(result.metadata.label, QStringLiteral("测试 \"双引号\""));
    }
    void stagingCopiesAndCleansUp() {
        QTemporaryDir dir;
        const auto input = dir.filePath(QStringLiteral("中文 😀.apk"));
        QFile file(input); QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("abc"); file.close();
        QString stagedPath;
        {
            const auto staged = zzapk::stageApk(input, {});
            QVERIFY2(staged.error.isEmpty(), qPrintable(staged.error));
            QVERIFY(staged.directory); stagedPath = staged.path;
            QFile copy(stagedPath); QVERIFY(copy.open(QIODevice::ReadOnly));
            QCOMPARE(copy.readAll(), QByteArray("abc"));
            QCOMPARE(QFileInfo(stagedPath).fileName(), "input.apk");
        }
        QVERIFY(!QFileInfo::exists(stagedPath)); QVERIFY(QFileInfo::exists(input));
        std::stop_source stop; stop.request_stop();
        QVERIFY(zzapk::stageApk(input, stop.get_token()).cancelled);
        QVERIFY(!zzapk::stageApk(dir.filePath("missing.apk"), {}).error.isEmpty());
    }
    void rejectInvalidOutput() {
        QVERIFY(!zzapk::parseBadging("ERROR: invalid APK\n").error.isEmpty());
        QVERIFY(!zzapk::parseBadging("uses-permission: name='org.fake'\n").error.isEmpty());
    }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "CoreTests.moc"
