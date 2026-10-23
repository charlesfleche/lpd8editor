#include "iomidi.h"

#include <QtTest>

class TestIOMIdi : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void test_handle();
    void test_clientId();
    void test_portId();

private:
    IOMidi *io{};
    QByteArray skipReason;
};

void TestIOMIdi::initTestCase()
{
    io = nullptr;
    try {
        io = new IOMidi(this);
    } catch (const std::exception &e) {
        // Not an error: CI machines usually have no /dev/snd/seq. Keep the
        // reason so the skip message says what happened.
        skipReason = QByteArray("No ALSA sequencer available: ") + e.what();
    }
}

void TestIOMIdi::init()
{
    if (!io) {
        QSKIP(skipReason.constData());
    }
}

void TestIOMIdi::test_handle()
{
    snd_seq_t *seq = io->handle();
    QVERIFY(seq);

    snd_seq_client_info_t *info = nullptr;
    snd_seq_client_info_alloca(&info);
    QCOMPARE(snd_seq_get_client_info(seq, info), 0);
    QVERIFY(!QString(snd_seq_client_info_get_name(info)).isNull());
}

void TestIOMIdi::test_clientId()
{
    QVERIFY(io->clientId() > 0);
}

void TestIOMIdi::test_portId()
{
    // The port must exist on our client, with the name IOMidi gives it.
    snd_seq_port_info_t *info = nullptr;
    snd_seq_port_info_alloca(&info);
    QCOMPARE(snd_seq_get_port_info(io->handle(), io->portId(), info), 0);
    QCOMPARE(QString::fromUtf8(snd_seq_port_info_get_name(info)),
             QCoreApplication::applicationName());
}

QTEST_GUILESS_MAIN(TestIOMIdi)

#include "iomidi_test.moc"
