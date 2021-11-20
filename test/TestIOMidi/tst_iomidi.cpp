#include "iomidi.h"
#include "db.h"

#include <QtTest>

class TestIOMIdi : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void test_handle();
    void test_clientId();
    void test_portId();

private:
    IOMidi* io;
};

void TestIOMIdi::initTestCase() {
    io = nullptr;
    try {
        io = new IOMidi(this);
    }
    catch (const std::exception&) {
    }
}

void TestIOMIdi::test_handle()
{
    if (!io) {
        return;
    }

    snd_seq_t* seq = io->handle();
    QVERIFY(seq);

    snd_seq_client_info_t *info;
    snd_seq_client_info_alloca(&info);
    QCOMPARE(snd_seq_get_client_info(seq, info), 0);
    QVERIFY(!QString(snd_seq_client_info_get_name(info)).isNull());
}

void TestIOMIdi::test_clientId()
{
    if (!io) {
        return;
    }

    QVERIFY(io->clientId() > 0);
}

void TestIOMIdi::test_portId()
{
    if (!io) {
        return;
    }

    QVERIFY(io->clientId() > 0);
}

QTEST_GUILESS_MAIN(TestIOMIdi)

#include "tst_iomidi.moc"
