#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QMessageBox>

#include <memory>

// Message boxes whose text is always plain text.
//
// QMessageBox guesses whether its text is HTML (Qt::mightBeRichText), and
// the messages of this application contain file paths, file names and
// program names, which come from outside. A path through a folder named
// '<img src="' renders as an image tag, and Qt then reads the file it
// names: a FIFO there froze the window, /dev/zero would fill the memory.
// Use these instead of the static QMessageBox functions for any text that
// isn't entirely ours.

std::unique_ptr<QMessageBox> plainTextMessageBox(QMessageBox::Icon icon,
                                                 const QString &title,
                                                 const QString &text,
                                                 QWidget *parent);

// Modal, like the static QMessageBox functions: they return when the user
// closes the box. A test keeps those out of src/ (cmake/CheckPlainText.cmake).
void showInformation(QWidget *parent, const QString &title, const QString &text);
void showWarning(QWidget *parent, const QString &title, const QString &text);
void showCritical(QWidget *parent, const QString &title, const QString &text);

#endif // MESSAGEBOX_H
