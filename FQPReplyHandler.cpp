#include "FQPReplyHandler.h"

#include "FQPClient.h"
#include "FQPRequest.h"

#include <QAuthenticator>
#include <QDebug>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QIODevice>
#include <QThread>

#include <QCoreApplication>

#include <functional>

FQPReplyHandler::FQPReplyHandler(QNetworkAccessManagerPtr accessManager,
                                 FQPRequestPtr request,
                                 const QStringList *resultParameters,
                                 QObject *parent):
    QObject(parent),
    _accessManager(accessManager),
    _request(request),
    _completed(false)
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
    delete _reply;
}

void
FQPReplyHandler::Request()
{
    // Store it because we may need to get the information for redirects, etc.
    QNetworkAccessManagerSharedPtr accessManager = _accessManager.lock();
    FQPRequestSharedPtr request = _request.lock();

    if (!accessManager || !request) {
        return;
    }
    QObject::connect(accessManager.get(),
                     &QNetworkAccessManager::authenticationRequired,
                     this,
                     &FQPReplyHandler::_OnAuthenticationRequired);
    QObject::connect(accessManager.get(),
                     &QNetworkAccessManager::finished,
                     this,
                     &FQPReplyHandler::_OnAccessManagerFinished);
    _reply = accessManager->sendCustomRequest(request->GetRequest(),
                                              request->GetMethod(),
                                              request->GetContent());
    QObject::connect(_reply,
                     static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
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
    //    _reply->abort();
    //request->GetContent());
    _buffer.clear();
    // QNetworkReply signals
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
FQPReplyHandler::_OnAccessManagerFinished(QNetworkReply * reply)
{
    if (reply->error()) {
        qDebug() << "FQPReplyHandler::_OnAccessManagerFinished() with "
                 << reply->error();
        qDebug() << "FQPReplyHandler::_OnAccessManagerFinished() error "
                 << _buffer;
    }
}

void
FQPReplyHandler::_OnFinished()
{
    if (_reply->error() == QNetworkReply::ProtocolUnknownError) {
        // Start a new request
        QVariant locationHeader = _reply->header(QNetworkRequest::LocationHeader);
        if (locationHeader.type() != QVariant::String) {
            throw FQPNetworkException();
        }
        QString locationString = locationHeader.toString();
        Request();
    } else {
        auto request = _request.lock();
        if (request) {
            _StoreCSRF(request->GetRequest().url());
            // Read the rest of the buffer, if there's anything left
        }
        _buffer.append(_reply->readAll());
        _EmitResults();
    }
    _completed = true;
}

void
FQPReplyHandler::_OnBytesReceived(qint64 bytesReceived, qint64 /*bytesTotal*/)
{
    //qDebug() << "FQPReplyHandler::_OnBytesReceived()" << bytesReceived;
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
    auto accessManager = _accessManager.lock();
    if (accessManager) {
        foreach(cookie, accessManager->cookieJar()->cookiesForUrl(baseUrl)) {
            if (cookie.name() == FQPClient::CSRFCookieName) {
                // Actully, it seems like the manager holds on to this, so as
                // long as the access manager doesn't go away, no one needs to
                // listen to this notice.
                emit CSRFTokenUpdated(cookie.value());
            }
        }
    }
}

QJsonDocument
FQPReplyHandler::_GetJsonFromContent(const QByteArray& content) const
{
    if (content.size() <= 0) {
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
        emit InterpretedReplyReceived(_reply->error(),
                                      QVariantList());
        break;
    case RawResults:
        emit RawReplyReceived(_reply->error(), jsonDoc);
        break;
    case InterpretedResults:
        if (jsonDoc.isNull() || jsonDoc.isEmpty()) {
            throw FQPNetworkException();
        }
        if (!jsonDoc.isObject()) {
            throw FQPNetworkException();
        }
        QVariantMap jsonObj = jsonDoc.object().toVariantMap();
        QVariantList vals;
        QStringList::const_iterator paramIterator;
        for (paramIterator = _resultParameters.constBegin() ;
             paramIterator != _resultParameters.constEnd() ;
             ++paramIterator) {
            vals.append(jsonObj[*paramIterator]);
        }
        emit InterpretedReplyReceived(_reply->error(), vals);
    }
}
