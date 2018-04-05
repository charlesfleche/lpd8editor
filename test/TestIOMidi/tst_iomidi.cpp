#include "iomidi.h"
#include "db.h"

#include <QtTest>

class TestIOMIdi : public QObject
{
    Q_OBJECT

private slots:
    void test_handle();
    void test_clientId();
    void test_portId();

private:
    IOMidi io;
};

void TestIOMIdi::test_handle()
{
    snd_seq_t* seq = io.handle();
    QVERIFY(seq);

    snd_seq_client_info_t *info;
    snd_seq_client_info_alloca(&info);
    QCOMPARE(snd_seq_get_client_info(seq, info), 0);
    QVERIFY(!QString(snd_seq_client_info_get_name(info)).isNull());
}

void TestIOMIdi::test_clientId()
{
    QVERIFY(io.clientId() > 0);
}

void TestIOMIdi::test_portId()
{
    QVERIFY(io.clientId() >= 0);
}

QTEST_APPLESS_MAIN(TestIOMIdi)

#include "tst_iomidi.moc"
