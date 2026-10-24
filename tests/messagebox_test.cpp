#include "app.h"
#include "messagebox.h"

#include <QApplication>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>

#include <QtTest>

#include <sys/stat.h>

class TestMessageBox : public QObject
{
    Q_OBJECT

private slots:
    void test_textIsPlain();
    void test_appDialogsArePlain();
};

void TestMessageBox::test_textIsPlain()
{
    // The message for a file imported from a folder named '<img src="'.
    // Shown as HTML, the box would read the 'image', a FIFO here, and wait
    // forever for a writer: this test hung before the fix.
    const QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString fifo = dir.filePath("image");
    QCOMPARE(::mkfifo(QFile::encodeName(fifo).constData(), 0600), 0);
    const QString text =
            QStringLiteral("/home/u/<img src=\"%1\">.lpd8 is not an LPD8 program.").arg(fifo);
    QVERIFY(Qt::mightBeRichText(text)); // what QMessageBox would have guessed

    const auto box = plainTextMessageBox(QMessageBox::Warning, "Import", text, nullptr);
    QCOMPARE(box->textFormat(), Qt::PlainText);
    QCOMPARE(box->text(), text);

    box->show();
    QVERIFY(QTest::qWaitForWindowExposed(box.get()));
    box->close();
}

void TestMessageBox::test_appDialogsArePlain()
{
    // App's dialogs (startup questions, notices, the confirmation to close
    // without saving) show messages naming the programs file, the old
    // database and the folders above them. Each is opened for real; a timer
    // records the format of the modal box and closes it.
    lpd8::MessageBoxDialogs dialogs;
    const QString text = QStringLiteral("/home/u/<b>x</b>/programs.json cannot be read.");
    QVERIFY(Qt::mightBeRichText(text));

    QList<Qt::TextFormat> formats;
    const auto closeTheBox = [&formats] {
        QTimer::singleShot(0, [&formats] {
            auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            QVERIFY(box != nullptr);
            formats.append(box->textFormat());
            box->reject();
        });
    };

    closeTheBox();
    dialogs.ask(text, {lpd8::AppDialogs::Answer::Retry});
    closeTheBox();
    dialogs.tell(nullptr, text);
    closeTheBox();
    dialogs.confirmCloseWithoutSaving(nullptr, text);

    QCOMPARE(formats, QList<Qt::TextFormat>({Qt::PlainText, Qt::PlainText, Qt::PlainText}));
}

QTEST_MAIN(TestMessageBox)

#include "messagebox_test.moc"
