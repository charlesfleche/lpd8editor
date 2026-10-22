#include "db.h"
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
};

void TestIOMIdi::initTestCase()
{
    io = nullptr;
    try {
        io = new IOMidi(this);
    } catch (const std::exception &) { }
}

void TestIOMIdi::init()
{
    if (!io) {
        QSKIP("No ALSA sequencer available (/dev/snd/seq)");
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
    QVERIFY(io->clientId() > 0);
}

QTEST_GUILESS_MAIN(TestIOMIdi)

#include "iomidi_test.moc"
