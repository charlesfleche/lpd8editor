#include "db.h"

#include <QSqlDatabase>

#include <QtTest>

class TestDB : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void test_1();
    void test_2();

private:
    QSqlDatabase db;
};

void TestDB::initTestCase() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
}

void TestDB::init() {
    db.open();
    initDb(db);
}

void TestDB::cleanup() {
    db.close();
}

void TestDB::test_1() {
    QCOMPARE(true, true);
}

void TestDB::test_2() {
    QCOMPARE(true, true);
}

QTEST_MAIN(TestDB)

#include "tst_db.moc"
