#include "utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QTemporaryFile>
#include <QTextStream>

#include <QtTest>

class TestDB : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
};

void TestDB::initTestCase()
{

}

void TestDB::cleanupTestCase()
{

}

QTEST_MAIN(TestDB)

#include "tst_db.moc"
