#include "messagebox.h"

std::unique_ptr<QMessageBox> plainTextMessageBox(QMessageBox::Icon icon,
                                                 const QString &title,
                                                 const QString &text,
                                                 QWidget *parent)
{
    auto box = std::make_unique<QMessageBox>(icon, title, text, QMessageBox::Ok, parent);
    box->setTextFormat(Qt::PlainText);
    return box;
}

void showInformation(QWidget *parent, const QString &title, const QString &text)
{
    plainTextMessageBox(QMessageBox::Information, title, text, parent)->exec();
}

void showWarning(QWidget *parent, const QString &title, const QString &text)
{
    plainTextMessageBox(QMessageBox::Warning, title, text, parent)->exec();
}

void showCritical(QWidget *parent, const QString &title, const QString &text)
{
    plainTextMessageBox(QMessageBox::Critical, title, text, parent)->exec();
}
