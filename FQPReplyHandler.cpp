#include "FQPReplyHandler.h"

#include "FQPClient.h"
#include "FQPRequest.h"

#include <QAuthenticator>
#include <QDebug>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QIODevice>

FQPReplyHandler::FQPReplyHandler(const QStringList *resultParameters,
                                 QObject *parent):
    QObject(parent)
{
    if (resultParameters) {
        if (resultParameters->size() > 0) {
            _resultsFormat = InterpretedResults;
        } else {
            _resultsFormat = RawResults;
        }
        _resultParameters = *resultParameters;
    } else {
        _resultsFormat = NoResults;
    }
}

FQPReplyHandler::~FQPReplyHandler()
{
    qDebug() << "~FQPReplyHandler: goodbye";
    delete _reply;
}

bool
FQPReplyHandler::Request(QNetworkAccessManagerSharedPtr accessManager,
                         FQPRequestSharedPtr request)
{
    // Store it because we may need to get the information for redirects, etc.
    if (_request != request) {
        _request = request;
    }
    if (_accessManager != accessManager) {
        _accessManager = accessManager;
    }
    QObject::connect(accessManager.get(),
                     &QNetworkAccessManager::authenticationRequired,
                     this,
                     &FQPReplyHandler::_OnAuthenticationRequired);
    qDebug() << "Sending request";
    _reply = accessManager->sendCustomRequest(request->GetRequest(),
                                              request->GetMethod(),
                                              request->GetContent());
    //request->GetContent());
    qDebug() << "Request sent. connecting for reply";
    _buffer.clear();
    // QNetworkReply signals
    QObject::connect(_reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                     this, &FQPReplyHandler::_OnError);
    QObject::connect(_reply, &QNetworkReply::finished,
                     this, &FQPReplyHandler::_OnFinished);
    QObject::connect(_reply, &QNetworkReply::downloadProgress,
                     this, &FQPReplyHandler::_OnBytesReceived);
    QObject::connect(_reply, &QNetworkReply::sslErrors,
                     this, &FQPReplyHandler::_OnSslErrors);

    // QIODevice signals
    QObject::connect(_reply, &QNetworkReply::readyRead,
                     this, &FQPReplyHandler::_OnReadyRead);

    return true;
}

void
FQPReplyHandler::_OnAuthenticationRequired(QNetworkReply * ,
                                           QAuthenticator * /*authenticator*/)
{
    qDebug() << "Authentication required. authenticating...";
    //authenticator->setUser("guest");
    //authenticator->setPassword("guest");
    // Send the request again
    //Request(_accessManager, _request);
}

void
FQPReplyHandler::_OnFinished()
{
    qDebug() << "FQPReplyHandler::_OnFinished";
    if (_reply->error() == QNetworkReply::ProtocolUnknownError) {
        // Start a new request
        QVariant locationHeader = _reply->header(QNetworkRequest::LocationHeader);
        if (locationHeader.type() != QVariant::String) {
            throw FQPNetworkException();
        }
        QString locationString = locationHeader.toString();
        Request(_accessManager, _request);
    } else {
        _StoreCSRF(_request->GetRequest().url());
        // Read the rest of the buffer, if there's anything left
        _buffer.append(_reply->readAll());
        _EmitResults();
    }
}

void
FQPReplyHandler::_OnBytesReceived(qint64 bytesReceived, qint64 /*bytesTotal*/)
{
    qDebug() << "FQPReplyHandler::_OnBytesReceived";
    if (bytesReceived <= 0) {
        qDebug() << "Nothing to read";
        return;
    }
    QByteArray bytesRead = _reply->readAll();
    if (bytesRead.size() == 0) {
        //qDebug() << "-1 returned from QIODevice::read()";
        //qDebug() << "0 bytes read from QIODevice::readAll()";
        return;
    } else if (bytesRead.size() != bytesReceived) {
        // This was garbage (on android, at least), so throw it out.
        qDebug() << "Read " << bytesRead << ", expected " << bytesReceived;
        //                 << "[" << buffer << "]";
    }
    _buffer.append(bytesRead);

}    

void
FQPReplyHandler::_OnReadyRead()
{
    qDebug() << "FQPReplyHandler::_OnReadyRead()";
    // We hack a little here, since this is a GET handler, which is only used for
    // single requests (not the check, then fetch), so we assume that the caller
    // wants to get this request and there isn't a fetch coming.
    //qDebug() << "ReadyRead content: " << _buffer;
    _buffer.append(_reply->readAll());
}

void
FQPReplyHandler::_OnError(QNetworkReply::NetworkError error)
{
    qDebug() << "FQPReplyHandler::_OnError()" << error;
                
}
// Handling SSL errors (Since android gives an error that iOS does not)
void
FQPReplyHandler::_OnSslErrors(const QList<QSslError>& errors)
{
    qDebug() << "Got SSL Errors: " << errors;
    // XXX - Hacky hack hack. Android fails on handshake error. This let's us
    // get by that. Should really do a better review on this. (We are using
    // a signed cert, but apparently, not trusted enough for android.)
    _reply->ignoreSslErrors(errors);
}

void
FQPReplyHandler::_StoreCSRF(const QUrl& baseUrl)
{
    QNetworkCookie cookie;
    foreach(cookie, _accessManager->cookieJar()->cookiesForUrl(baseUrl)) {
        if (cookie.name() == FQPClient::CSRFCookieName) {
            // Actully, it seems like the manager holds on to this, so as long
            // as the access manager doesn't go away, no one needs to listen to
            // this notice.
            emit CSRFTokenUpdated(cookie.value());
        }
    }
}

QJsonDocument
FQPReplyHandler::_GetJsonFromContent(const QByteArray& content) const
{
    if (content.size() <= 0) {
        qDebug() << "Buffer empty. returning";
        return QJsonDocument();
    }

    QByteArray cleanedContent = content;
    unsigned int desiredSize = cleanedContent.size() -1;
    //qDebug() << "Raw: <<" << _buffer.data() << ">>";
    while ((cleanedContent.at(desiredSize) != '}') &&
           (cleanedContent.at(desiredSize) != ']')) {
        // Remove crap at the end. Namely, the ';' that the server sticks on.
        // That's never valid JSON, so F-U rabbitmq management plugin.
        cleanedContent.chop(1);
        desiredSize--;
    }
    //qDebug() << "Trunc: <<" << cleanedContent << ">>";
    return QJsonDocument::fromJson(cleanedContent);
}

void
FQPReplyHandler::_EmitResults() {
    QJsonDocument jsonDoc = _GetJsonFromContent(_buffer);
    // Does the client care about the data we get back?
    switch (_resultsFormat) {
    case NoResults:
        emit InterpretedReplyReceived(QVariantList());
        break;
    case RawResults:
        emit RawReplyReceived(jsonDoc);
        break;
    case InterpretedResults:
        if (jsonDoc.isNull() || jsonDoc.isEmpty()) {
            throw FQPNetworkException();
        }
        qDebug() << "Results: " << jsonDoc;
        if (!jsonDoc.isObject()) {
            throw FQPNetworkException();
        }
        QVariantMap jsonObj = jsonDoc.object().toVariantMap();
        QVariantList vals;
        QStringList::const_iterator paramIterator;
        qDebug() << "Pulling " << _resultParameters << " out of results";
        for (paramIterator = _resultParameters.constBegin() ;
             paramIterator != _resultParameters.constEnd() ;
             ++paramIterator) {
            vals.append(jsonObj[*paramIterator]);
        }
        emit InterpretedReplyReceived(vals);
    }
}