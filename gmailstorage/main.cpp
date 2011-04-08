#include "mailsender.h"

#include <QDebug>

int main(int argc, char** argv)
{
    MailSender sender("smtp.gmail.com", "zanetteatest@gmail.com", QStringList("zanettea@gmail.com"), "@SAK", "Enjoy");
    sender.setLogin("zanetteatest", "test4test");
    sender.setSsl(true);
    sender.setPort(587);
    sender.setAttachments(QStringList("/tmp/prova.txt"));
    if (sender.send()) {
    
    } else {
	qDebug() << sender.lastError() << sender.lastCmd() << sender.lastResponse();
    }
}