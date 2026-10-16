#include "core/ApkAnalyzer.h"
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class AnalyzerTests : public QObject {
    Q_OBJECT
    QTemporaryDir dir;
    QString makeApk(const QString &name, const QByteArray &content = "abc") {
        const auto path = dir.filePath(name + ".apk");
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size()) return {};
        return path;
    }
    QString tools() const { return QCoreApplication::applicationDirPath(); }
private slots:
    void success() {
        zzapk::ApkAnalyzer analyzer(tools());
        QSignalSpy done(&analyzer, &zzapk::ApkAnalyzer::finished);
        QVERIFY(analyzer.analyze(makeApk(QStringLiteral("中文 空格")), zzapk::ApkTool::Aapt2));
        QTRY_COMPARE(done.size(), 1);
        auto r = qvariant_cast<zzapk::AnalysisResult>(done[0][0]);
        QVERIFY2(r.error.isEmpty(), qPrintable(r.error));
        QCOMPARE(r.md5, "900150983CD24FB0D6963F7D28E17F72");
        QCOMPARE(r.metadata.packageName, QStringLiteral("中文 空格"));
        QVERIFY(r.diagnostics.contains("tool warning")); QVERIFY(!analyzer.isBusy());
    }
    void processErrors_data() {
        QTest::addColumn<QByteArray>("mode");
        QTest::newRow("nonzero") << QByteArray("error");
        QTest::newRow("malformed") << QByteArray("malformed");
        QTest::newRow("timeout") << QByteArray("slow");
        QTest::newRow("output-limit") << QByteArray("flood");
    }
    void processErrors() {
        QFETCH(QByteArray, mode);
        zzapk::ApkAnalyzer analyzer(tools(), nullptr, mode == "slow" ? 50 : 5000);
        QSignalSpy done(&analyzer, &zzapk::ApkAnalyzer::finished);
        QVERIFY(analyzer.analyze(makeApk("failure", mode), zzapk::ApkTool::Aapt2));
        QTRY_COMPARE(done.size(), 1);
        const auto r = qvariant_cast<zzapk::AnalysisResult>(done[0][0]);
        QVERIFY(!r.error.isEmpty()); QVERIFY(!r.cancelled); QVERIFY(!analyzer.isBusy());
        if (mode == "error") QVERIFY(r.diagnostics.contains("invalid archive"));
    }
    void missingToolAndInvalidInput() {
        zzapk::ApkAnalyzer analyzer(dir.path());
        QSignalSpy done(&analyzer, &zzapk::ApkAnalyzer::finished);
        QSignalSpy rejected(&analyzer, &zzapk::ApkAnalyzer::rejected);
        QVERIFY(!analyzer.analyze(dir.filePath("missing.apk"), zzapk::ApkTool::Aapt));
        QCOMPARE(rejected.size(), 1); QVERIFY(!analyzer.isBusy());
        QVERIFY(analyzer.analyze(makeApk("valid"), zzapk::ApkTool::Aapt));
        QTRY_COMPARE(done.size(), 1);
        QVERIFY(!qvariant_cast<zzapk::AnalysisResult>(done[0][0]).error.isEmpty());
    }
    void replaceAndCancel() {
        zzapk::ApkAnalyzer analyzer(tools());
        QSignalSpy done(&analyzer, &zzapk::ApkAnalyzer::finished);
        QVERIFY(analyzer.analyze(makeApk("old", "slow"), zzapk::ApkTool::Aapt));
        QVERIFY(analyzer.analyze(makeApk("new"), zzapk::ApkTool::Aapt2));
        QTRY_COMPARE(done.size(), 2);
        QVERIFY(qvariant_cast<zzapk::AnalysisResult>(done[0][0]).cancelled);
        QCOMPARE(qvariant_cast<zzapk::AnalysisResult>(done[1][0]).metadata.packageName, "new");
        QTest::qWait(100); QCOMPARE(done.size(), 2);
        QVERIFY(analyzer.analyze(makeApk("cancel", "slow"), zzapk::ApkTool::Aapt));
        analyzer.cancel();
        QCOMPARE(done.size(), 3); QVERIFY(qvariant_cast<zzapk::AnalysisResult>(done[2][0]).cancelled);
        QVERIFY(!analyzer.isBusy());
    }
    void closeWhileRunning() {
        auto analyzer = std::make_unique<zzapk::ApkAnalyzer>(tools());
        QVERIFY(analyzer->analyze(makeApk("closing", "slow"), zzapk::ApkTool::Aapt));
        QElapsedTimer elapsed; elapsed.start();
        analyzer.reset();
        QVERIFY2(elapsed.elapsed() < 1000, "关闭窗口不应等待外部工具自然退出");
    }
};
QTEST_GUILESS_MAIN(AnalyzerTests)
#include "AnalyzerTests.moc"
