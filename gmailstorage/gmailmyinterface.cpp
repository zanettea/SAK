#include "gmailmyinterface.h"

#include "mailsender.h"

#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <QResource>
#include <QtGui>

#include <QFuture>
#include <qtconcurrentrun.h>

GmailPyInterface::GmailPyInterface()
{
    m_sender=0;
}

GmailPyInterface::~GmailPyInterface()
{
}


bool GmailPyInterface::login()
{
    bool goon=true;
    while(m_user == "" || m_pass == "") {
        QDialog d;
        d.setModal(true);
        QLineEdit* user(new QLineEdit);
        QLineEdit* pass(new QLineEdit);
        pass->setEchoMode(QLineEdit::Password);
        QVBoxLayout* layout(new QVBoxLayout);
        d.setLayout(layout);
        layout->addWidget(new QLabel("Enter you gmail account:"));
        QGridLayout* glayout(new QGridLayout());
        glayout->addWidget(new QLabel("Username:"),0,0);
        glayout->addWidget(user,0,1);
        glayout->addWidget(new QLabel("Password:"),1,0);
        glayout->addWidget(pass,1,1);
        layout->addLayout(glayout);
        QHBoxLayout* buttons(new QHBoxLayout);
        QPushButton* okbutton(new QPushButton("Ok"));
        QPushButton* cancelbutton(new QPushButton("Cancel"));
        QObject::connect(okbutton, SIGNAL(clicked()), &d, SLOT(accept()));
        QObject::connect(cancelbutton, SIGNAL(clicked()), &d, SLOT(reject()));
        buttons->addWidget(okbutton);
        buttons->addWidget(cancelbutton);
        layout->addLayout(buttons);
        d.exec();
        if (d.result() == QDialog::Accepted) {
            if (user->text().isEmpty() || pass->text().isEmpty()) continue;
            m_user = user->text();
            m_pass = pass->text();
            break;
        } else return false;
    }
    delete m_sender;
    m_sender = 0;
    m_sender = new MailSender("smtp.gmail.com", m_user + "@gmail.com", QStringList(m_user + "@gmail.com"), "@SAK", "Enjoy!");
    m_sender->setSsl(true);
    m_sender->setPort(587);
    m_sender->setLogin(m_user, m_pass);
    return true;
}


void GmailPyInterface::storeTaskFiles(const QStringList& filePaths)
{
    bool overide=false;

    if (!login()) {
        if (overide)
            QApplication::restoreOverrideCursor();
        return;
    }


    QProgressDialog progress("Saving files to GMAIL...", "Abort Copy", 0, filePaths.size(), 0);
    progress.setWindowModality(Qt::WindowModal);
    int value=0;
    foreach(const QString& filePath, filePaths) {
        progress.setValue(++value);
        progress.setLabelText("Sending to gmail " + filePath);
        QString taskName = QFileInfo(filePath).baseName();

        if (!overide) {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            overide=true;
        }
        m_sender->setAttachments(QStringList(filePath));
        m_sender->setSubject(taskName + "@SAK");
        if (!m_sender->send()) {
            QMessageBox::warning(0, "Error!", "Error: " + m_sender->lastError() + "\nexecuting command: " + m_sender->lastCmd() + "\nResponse: " + m_sender->lastResponse());
            break;
        }
         if (progress.wasCanceled())
             break;
    }
    if (overide)
        QApplication::restoreOverrideCursor();
}


