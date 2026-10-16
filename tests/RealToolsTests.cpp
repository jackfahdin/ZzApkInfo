#include "core/ApkAnalyzer.h"
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class RealToolsTests : public QObject {
    Q_OBJECT
private slots:
    void sample_data() {
        QTest::addColumn<bool>("useAapt2");
        QTest::addColumn<QString>("fileName");
        QTest::newRow("aapt-unicode") << false << QStringLiteral("中文 路径 😀.APK");
        QTest::newRow("aapt2-unicode") << true << QStringLiteral("中文 路径 😀.APK");
        QTest::newRow("aapt-dash") << false << QStringLiteral("-example.apk");
        QTest::newRow("aapt2-dash") << true << QStringLiteral("-example.apk");
    }
    void sample() {
        QFETCH(bool, useAapt2);
        QTemporaryDir dir;
        QFETCH(QString, fileName);
        const auto path = dir.filePath(fileName);
        QVERIFY(QFile::copy(QStringLiteral(ZZAPK_FIXTURE), path));
        zzapk::ApkAnalyzer analyzer(QStringLiteral(ZZAPK_REAL_TOOLS));
        QSignalSpy done(&analyzer, &zzapk::ApkAnalyzer::finished);
        QVERIFY(analyzer.analyze(path, useAapt2 ? zzapk::ApkTool::Aapt2 : zzapk::ApkTool::Aapt));
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 10000);
        const auto result = qvariant_cast<zzapk::AnalysisResult>(done[0][0]);
        QVERIFY2(result.error.isEmpty(), qPrintable(result.error + result.diagnostics));
        QCOMPARE(result.metadata.label, QStringLiteral("示例 APK"));
        QCOMPARE(result.metadata.packageName, "org.zzapk.fixture");
        QCOMPARE(result.metadata.activity, "org.zzapk.fixture.MainActivity");
        QCOMPARE(result.metadata.versionName, "1.2.3");
        QCOMPARE(result.metadata.versionCode, "7");
        QCOMPARE(result.md5, "5BE15C255287D26F28142D46098415AD");
    }
    void brokenArchive() {
        QTemporaryDir dir; QFile file(dir.filePath("broken.apk"));
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write("not a zip"); file.close();
        zzapk::ApkAnalyzer analyzer(QStringLiteral(ZZAPK_REAL_TOOLS));
        QSignalSpy done(&analyzer, &zzapk::ApkAnalyzer::finished);
        QVERIFY(analyzer.analyze(file.fileName(), zzapk::ApkTool::Aapt2));
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 10000);
        const auto result = qvariant_cast<zzapk::AnalysisResult>(done[0][0]);
        QVERIFY(!result.error.isEmpty()); QVERIFY(!result.diagnostics.isEmpty());
        QVERIFY(result.metadata.packageName.isEmpty());
    }
};
QTEST_GUILESS_MAIN(RealToolsTests)
#include "RealToolsTests.moc"
