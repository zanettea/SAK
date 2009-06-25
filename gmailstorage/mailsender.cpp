#include "mailsender.h"
#include <QString>
#include <QSslSocket>
#include <QTextStream>
#include <QByteArray>
#include <QDateTime>
#include <QTextCodec>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <time.h>


static int dateswap(QString form, uint unixtime )
{
    QDateTime fromunix;
    fromunix.setTime_t(unixtime); 
    QString numeric = fromunix.toString((const QString)form);
    bool ok; 
    return (int)numeric.toFloat(&ok);
}

static QString encodeBase64( QString xml )
{
    QByteArray text;
    text.append(xml);
    return text.toBase64();
}


static QString TimeStampMail()
{
    /* mail rtf Date format! http://www.faqs.org/rfcs/rfc788.html */
    uint unixtime = (uint)time( NULL );
    QDateTime fromunix;
    fromunix.setTime_t(unixtime); 
    QStringList RTFdays = QStringList() << "giorno_NULL" << "Mon" << "Tue" << "Wed" << "Thu" << "Fri" << "Sat" << "Sun";
    QStringList RTFmonth = QStringList() << "mese_NULL" << "Jan" << "Feb" << "Mar" << "Apr" << "May" << "Jun" << "Jul" << "Aug" << "Sep" << "Oct" << "Nov" << "Dec";
    QDate timeroad(dateswap("yyyy",unixtime),dateswap("M",unixtime),dateswap("d",unixtime));
    QStringList rtfd_line;
    rtfd_line.clear();
    rtfd_line.append("Date: ");
    rtfd_line.append(RTFdays.at(timeroad.dayOfWeek()));
    rtfd_line.append(", ");
    rtfd_line.append(QString::number(dateswap("d",unixtime)));
    rtfd_line.append(" ");
    rtfd_line.append(RTFmonth.at(dateswap("M",unixtime)));
    rtfd_line.append(" ");
    rtfd_line.append(QString::number(dateswap("yyyy",unixtime)));
    rtfd_line.append(" ");
    rtfd_line.append(fromunix.toString("hh:mm:ss"));
    rtfd_line.append("");

    return QString(rtfd_line.join(""));
}

static QString createBoundary()
{
    QByteArray hash = QCryptographicHash::hash(QString(QString::number(qrand())).toUtf8(),QCryptographicHash::Md5);
    QString boundary = hash.toHex();
    boundary.truncate(26);
    boundary.prepend("----=_NextPart_");
    return boundary;
}


MailSender::MailSender(const QString &smtpServer, const QString &from, const QStringList &to, const QString &subject, const QString &body)
{
    setSmtpServer(smtpServer);
    setPort(25);
    setTimeout(30000);
    setFrom(from);
    setTo(to);
    setSubject(subject);
    setBody(body);
    setPriority (normal);
    setContentType(text);
    setEncoding(_8bit);
    setISO(utf8);
    setSsl(false);
}

MailSender::~MailSender()
{
    if(_socket) {
        delete _socket;
    }
}

void MailSender::setFrom(const QString &from)
{
    _from = from;
    _fromName = from;
    _replyTo = from;
}

void MailSender::setISO(ISO iso)
{
    switch(iso) {
      case iso88591: _iso = "iso-8859-1"; _bodyCodec = "ISO 8859-1"; break;
      case utf8:     _iso = "utf-8"; _bodyCodec = "UTF-8"; break;
    }
}

void MailSender::setEncoding(Encoding encoding)
{
    switch(encoding) {
      case _7bit:     _encoding = "7bit"; break;
      case _8bit:     _encoding = "8bit"; break;
      case base64:    _encoding = "base64"; break;
    }
}

QString MailSender::contentType()
{
    switch(_contentType) {
      case html:            return "text/html";
      case multipartmixed:  return "multipart/mixed";
      case text:
      default:              return "text/plain";
    }
}

QString MailSender::mailData()
{
    QString data;
 
    QString boundary1 = createBoundary();
    QString boundary2 = createBoundary();

    QByteArray hash = QCryptographicHash::hash(QString(QString::number(qrand())).toUtf8(),QCryptographicHash::Md5);
    QString id = hash.toHex();
    data.append("Message-ID: " + id + "@" + QHostInfo::localHostName() + "\n");

    data.append("From: \"" + _fromName + "\"");
    data.append(" <" + _from + ">\n");

    if ( _to.count() > 0 ) {
        data.append("To: ");
        for ( int i = 0; i < _to.count(); i++ ) {
            data.append("<" + _to.at(i) + ">" + ",");
        }
        data.append("\n"); 
    }

    if ( _cc.count() > 0 ) {
        data.append("Cc: "); 
        for ( int i = 0; i < _cc.count(); i++ ) {
            data.append(_cc.at(i) + ",");
            if(i < _cc.count()-1) {
                data.append(",");
            }
        }
        data.append("\n");
    }

    data.append("Subject: " + _subject + "\n");
    data.append(TimeStampMail() + "\n");

    data.append("MIME-Version: 1.0\n");
    data.append("Content-Type: Multipart/Mixed; boundary=\"" + boundary1 + "\"\n");

    switch(_priority) {
      case low:
        data.append("X-Priority: 5\n");
        data.append("Priority: Non-Urgent\n");
        data.append("X-MSMail-Priority: Low\n");
        data.append("Importance: low\n");
        break;
      case high:
        data.append("X-Priority: 1\n");
        data.append("Priority: Urgent\n");
        data.append("X-MSMail-Priority: High\n");
        data.append("Importance: high\n");
        break;
      default:
        data.append("X-Priority: 3\n");
        data.append("    X-MSMail-Priority: Normal\n");
    }

    data.append("X-Mailer: QT4\r\n");  

    if ( ! _confirmTo.isEmpty() ) {
        data.append("Disposition-Notification-To: " + _confirmTo + "\n");
    }

    if ( ! _replyTo.isEmpty() && _confirmTo != _from ) {
        data.append("Reply-to: " + _replyTo + "\n");
        data.append("Return-Path: <" + _replyTo + ">\n");
    }

    data.append("\n");

    data.append("This is a multi-part message in MIME format.\r\n\n");
    data.append("--" + boundary1 + "\n");
    data.append("Content-Type: multipart/alternative; boundary=\"" + boundary2 + "\"\n\n\n");
    data.append("--" + boundary2 + "\n");

    data.append("Content-Type: " + contentType() + ";\n");
    data.append("  charset=" + _iso + "\r\n");
    data.append("Content-Transfer-Encoding: " + _encoding + "\r\n");
    data.append("\r\n\n");

    if ( _contentType == html ) {
        data.append("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\r\n");
        data.append("<HTML><HEAD>\r\n");
        data.append("<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=" + _iso + "\">\r\n");
        data.append("<META content=\"MSHTML 6.00.2900.2802\" name=GENERATOR>\r\n");
        data.append("<STYLE></STYLE>\r\n");
        data.append("</head>\r\n");
        data.append("<body bgColor=#ffffff>\r\n");
    }

    QByteArray encodedBody(_body.toLatin1()); // = array;
    QTextCodec *codec = QTextCodec::codecForName(_bodyCodec.toLatin1()); 
    data.append(codec->toUnicode(encodedBody) + "\r\n");

    if ( _contentType == html ) {
        data.append("<DIV>&nbsp;</DIV></body></html>\r\n\n");
        data.append("--" + boundary2 + "--\r\n\n");
    }

    if ( _attachments.count() > 0 ) {
        for ( int i = 0; i < _attachments.count(); i++) {
            data.append("--" + boundary1 + "\n");
            QFileInfo fileinfo(_attachments.value(i));
            QString type = getMimeType(fileinfo.suffix());
            data.append("Content-Type: " + type + ";\n");
            data.append("  name=" + fileinfo.fileName() + "\n");

            QString tt = fileinfo.fileName();

            data.append("Content-Transfer-Encoding: base64\n");
            data.append("Content-Disposition: attachment\n");
            data.append("  filename=" + fileinfo.fileName() + "\n\n");
            QString encodedFile;
            QString f =_attachments.value(i);
            QFile file(_attachments.value(i));
            file.open(QIODevice::ReadOnly);
            QDataStream in(&file);    
            quint8 a;
            char c;
            QString b;
            while ( ! in.atEnd() ) {
                in >> a;
                c = a;    
                b.append(c);
            }
            encodedFile = encodeBase64(b);
            data.append(encodedFile);
            data.append("\r\n\n");
        }
        data.append("--" + boundary1 + "--\r\n\n");
    }

    _lastMailData = data;
    return data; 
}

QString MailSender::getMimeType(QString ext)
{
//texte
    if (ext == "txt")			return "text/plain";
    if (ext == "htm" || ext == "html")	return "text/html";
    if (ext == "css")			return "text/css";
//Images
    if (ext == "png")			return "image/png";
    if (ext == "gif")			return "image/gif";
    if (ext == "jpg" || ext == "jpeg")	return "image/jpeg";
    if (ext == "bmp")			return "image/bmp";
    if (ext == "tif")			return "image/tiff";
//Archives
    if (ext == "bz2")			return "application/x-bzip";
    if (ext == "gz")			return "application/x-gzip";
    if (ext == "tar" )			return "application/x-tar";
    if (ext == "zip" )			return "application/zip";
//Audio
    if ( ext == "aif" || ext == "aiff")	return "audio/aiff";
    if ( ext == "mid" || ext == "midi")	return "audio/mid";
    if ( ext == "mp3")			return "audio/mpeg";
    if ( ext == "ogg")			return "audio/ogg";
    if ( ext == "wav")			return "audio/wav";
    if ( ext == "wma")			return "audio/x-ms-wma";
//Video
    if ( ext == "asf" || ext == "asx")	return "video/x-ms-asf";
    if ( ext == "avi")			return "video/avi";
    if ( ext == "mpg" || ext == "mpeg")	return "video/mpeg";
    if ( ext == "wmv")			return "video/x-ms-wmv";
    if ( ext == "wmx")			return "video/x-ms-wmx";
//XML
    if ( ext == "xml")			return "text/xml";
    if ( ext == "xsl")			return "text/xsl";
//Microsoft
    if ( ext == "doc" || ext == "rtf")	return "application/msword";
    if ( ext == "xls")			return "application/excel";
    if ( ext == "ppt" || ext == "pps")	return "application/vnd.ms-powerpoint";
//Adobe
    if ( ext == "pdf")			return "application/pdf";
    if ( ext == "ai" || ext == "eps")	return "application/postscript";
    if ( ext == "psd")			return "image/psd";
//Macromedia
    if ( ext == "swf")			return "application/x-shockwave-flash";
//Real
    if ( ext == "ra")			return "audio/vnd.rn-realaudio";
    if ( ext == "ram")			return "audio/x-pn-realaudio";
    if ( ext == "rm")			return "application/vnd.rn-realmedia";
    if ( ext == "rv")			return "video/vnd.rn-realvideo";
//Other
    if ( ext == "exe")			return "application/x-msdownload";
    if ( ext == "pls")			return "audio/scpls";
    if ( ext == "m3u")			return "audio/x-mpegurl";

    return "text/plain"; // default
}

void MailSender::errorReceived(QAbstractSocket::SocketError socketError)
{
    QString msg;

    switch(socketError) {
        case QAbstractSocket::ConnectionRefusedError: msg = "ConnectionRefusedError"; break;
        case QAbstractSocket::RemoteHostClosedError: msg = "RemoteHostClosedError"; break;
        case QAbstractSocket::HostNotFoundError: msg = "HostNotFoundError"; break;
        case QAbstractSocket::SocketAccessError: msg = "SocketAccessError"; break;
        case QAbstractSocket::SocketResourceError: msg = "SocketResourceError"; break;
        case QAbstractSocket::SocketTimeoutError: msg = "SocketTimeoutError"; break;
        case QAbstractSocket::DatagramTooLargeError: msg = "DatagramTooLargeError"; break;
        case QAbstractSocket::NetworkError: msg = "NetworkError"; break;
        case QAbstractSocket::AddressInUseError: msg = "AddressInUseError"; break;
        case QAbstractSocket::SocketAddressNotAvailableError: msg = "SocketAddressNotAvailableError"; break;
        case QAbstractSocket::UnsupportedSocketOperationError: msg = "UnsupportedSocketOperationError"; break;
        case QAbstractSocket::ProxyAuthenticationRequiredError: msg = "ProxyAuthenticationRequiredError"; break;
        default: msg = "Unknown Error";
    }

    error("Socket error [" + msg + "]");
}


bool MailSender::send()
{
    _lastError = "";

    if(_socket) {
        delete _socket;
    }

    _socket = _ssl ? new QSslSocket(this) : new QTcpSocket(this); 
 
    connect( _socket, SIGNAL( error( QAbstractSocket::SocketError) ), this, SLOT( errorReceived( QAbstractSocket::SocketError ) ) );
    connect( _socket, SIGNAL( proxyAuthenticationRequired(const QNetworkProxy & , QAuthenticator *) ), this, SLOT(proxyAuthentication(const QNetworkProxy &, QAuthenticator * ) ) );

    bool auth = ! _login.isEmpty();

    _socket->connectToHost( _smtpServer, _port );

    if( !_socket->waitForConnected( _timeout ) ) {
        error("Time out connecting host");
        return false;
    }

    if(!read("220")) {
        return false;
    }

    if ( !sendCommand("EHLO there", "250") ) {
        if ( !sendCommand("HELO there", "250") ) {
            return false;
        }
    }

    if(_ssl) {
        if ( !sendCommand("STARTTLS", "220") ) {
            return false;
        }
        QSslSocket *pssl = qobject_cast<QSslSocket *>(_socket);
        if(pssl == 0) {
            error("internal error casting to QSslSocket");
        	return false;
        }
        pssl->startClientEncryption ();
    }


    if ( auth ) {
        if( !sendCommand("AUTH LOGIN", "334") ) {
            return false;
        }
        if( !sendCommand(encodeBase64(_login), "334") ) {
            return false;
        }
        if( !sendCommand(encodeBase64(_password), "235") ) {
            return false;
        }
    }

    if( !sendCommand(QString::fromLatin1("MAIL FROM:<") +_from + QString::fromLatin1(">"), "250") ) {
        return false;
    }

    QStringList recipients = _to + _cc + _bcc;
    for (int i=0; i< recipients.count(); i++) {
        if( !sendCommand(QString::fromLatin1("RCPT TO:<") + recipients.at(i) + QString::fromLatin1(">"), "250") ) {
            return false;
        }
    }

    if( !sendCommand(QString::fromLatin1("DATA"), "354") ) {
        return false;
    }
    if( !sendCommand(mailData() + QString::fromLatin1("\r\n."), "250") ) {
        return false;
    }
    if( !sendCommand(QString::fromLatin1("QUIT"), "221") ) {
        return false;
    }

    _socket->disconnectFromHost();
    return true;
}

bool MailSender::read(const QString &waitfor)
{
    if( ! _socket->waitForReadyRead( _timeout ) ) {
        error("Read timeout");
        return false;
    }

    if( !_socket->canReadLine() ) {
        error("Can't read");
        return false;
    }

    QString responseLine;

    do {
        responseLine = _socket->readLine();
    } while( _socket->canReadLine() && responseLine[3] != ' ' );

    _lastResponse = responseLine;

    QString prefix = responseLine.left(3);
    bool isOk = (prefix == waitfor);
    if(!isOk) {
        error("waiting for " + waitfor + ", received " + prefix);
    }

    return isOk;
}


bool MailSender::sendCommand(const QString &cmd, const QString &waitfor)
{
    QTextStream t(_socket);
    t << cmd + "\r\n";
    t.flush();

    _lastCmd = cmd;

    return read(waitfor);
}

void MailSender::error(const QString &msg)
{
    _lastError = msg;
}


void MailSender::proxyAuthentication(const QNetworkProxy &, QAuthenticator * authenticator)
{
    *authenticator = _authenticator;
}

void MailSender::setProxyAuthenticator(const QAuthenticator &authenticator)
{
    _authenticator = authenticator;
}

