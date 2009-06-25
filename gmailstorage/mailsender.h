#ifndef MAILSENDER_H
#define MAILSENDER_H

#include <QString>
#include <QStringList>
#include <QObject>
#include <QTcpSocket>
#include <QPointer>
#include <QAuthenticator>

class MailSender : public QObject
{
    Q_OBJECT

public:

enum Priority {high, normal, low};
enum ContentType {text, html, multipartmixed};
enum Encoding {_7bit, _8bit, base64};
enum ISO {utf8, iso88591};

    MailSender(const QString &smtpServer, const QString &from, const QStringList &to, const QString &subject, const QString &body);
    ~MailSender();
    bool send();
    QString lastError() {return _lastError;}
    QString lastCmd() {return _lastCmd;}
    QString lastResponse() {return _lastResponse;}
    QString lastMailData() {return _lastMailData;}

    void setSmtpServer (const QString &smtpServer) 	{_smtpServer = smtpServer;}
    void setPort (int port)					        {_port = port;}
    void setTimeout (int timeout)				    {_timeout = timeout;}
    void setLogin (const QString &login, const QString &passwd)		{_login = login; _password = passwd;}
    void setSsl(bool ssl)                           {_ssl = ssl;}
    void setCc (const QStringList &cc) 				{_cc = cc;}
    void setBcc (const QStringList &bcc) 			{_bcc = bcc;}
    void setAttachments (const QStringList &attachments)	{_attachments = attachments;}
    void setReplyTo (const QString &replyTo) 		{_replyTo = replyTo;}
    void setPriority (Priority priority) 			{_priority = priority;}
    void setFrom (const QString &from);
    void setTo (const QStringList &to) 				{_to = to;}
    void setSubject (const QString &subject) 		{_subject = subject;}
    void setBody (const QString &body) 				{_body = body;}
    void setFromName (const QString &fromName)      {_fromName = fromName;}
    void setContentType(ContentType contentType)	{_contentType = contentType;}
    void setISO(ISO iso);
    void setEncoding(Encoding encoding);
    void setProxyAuthenticator(const QAuthenticator &authenticator);

private slots:
    void errorReceived(QAbstractSocket::SocketError socketError);
    void proxyAuthentication(const QNetworkProxy & proxy, QAuthenticator * authenticator);

private:

    QString getMimeType(QString ext);
    QString mailData();
    QString contentType();
    bool read(const QString &waitfor);
    bool sendCommand(const QString &cmd, const QString &waitfor);
    void error(const QString &msg);

    QString	    _smtpServer;
    int         _port;
    int         _timeout;
    QString     _login;
    QString     _password;
    QPointer<QTcpSocket>  _socket;
    bool        _ssl;
    QAuthenticator _authenticator;
    QString     _lastError;
    QString     _lastCmd;
    QString     _lastResponse;
    QString     _lastMailData;

    QString	    _from;
    QStringList	_to;
    QString	    _subject;
    QString	    _body;
    QStringList _cc;
    QStringList _bcc;
    QStringList _attachments;
    QString	    _fromName;
    QString	    _replyTo;
    Priority	_priority;
    ContentType _contentType;
    QString	    _encoding;
    QString	    _iso;
    QString	    _bodyCodec;
    QString	    _confirmTo;
};

#endif
