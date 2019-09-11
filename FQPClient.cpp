#include "FQPClient.h"

#include "FQPRequest.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkAccessManager>

const QByteArray FQPClient::CSRFCookieName = "csrftoken";
const QByteArray FQPClient::CSRFHeaderName = "X-CSRFTOKEN";

FQPClient::FQPClient(const QUrl& baseUrl, QObject *parent) :
    QThread(parent),
    _baseUrl(baseUrl),
    _accessManager(_InitAccessManager())
{
    _AppendSlashToBaseIfNecessary();
}

void
FQPClient::run()
{
    // We just re-initialize the access manager so it "belongs" to the
    // new thread.
    _accessManager = _InitAccessManager();
    // _eventLoop->exec();
    exec();
}

bool
FQPClient::IsNetworkAccessible() const
{
    return _accessManager->networkAccessible() ==
        QNetworkAccessManager::Accessible;
}

QNetworkAccessManagerSharedPtr
FQPClient::_InitAccessManager()
{
    QNetworkAccessManager* accessManager = new QNetworkAccessManager();

    connect(accessManager,
            &QNetworkAccessManager::networkAccessibleChanged,
            this, &FQPClient::_OnNetworkAccessibleChanged);

    return QNetworkAccessManagerSharedPtr(accessManager);
}

void
FQPClient::_QueueRequest(const FQPRequestSharedPtr& request,
                         const FQPReplyHandlerSharedPtr& reply)
{
    _requestQueue.enqueue(std::make_pair(request, reply));
    QThread* accessManagerThread = _accessManager->thread();

    connect(reply.get(),&FQPReplyHandler::CSRFTokenUpdated,
            this, &FQPClient::_OnCSRFTokenUpdated);
    if (accessManagerThread != thread()) {
        // If we're not multithreaded, moving the thread is a noop, but let's
        // save a little work and only move if we're in a thread.
        qDebug() << "request and reply in different thread. Moving";
        reply->moveToThread(accessManagerThread);
        request->moveToThread(accessManagerThread);
        // Run the request in a separate thread.
        QMetaObject::invokeMethod(reply.get(),"Request", Qt::AutoConnection);
    } else {
        reply->Request();
    }

}

void
FQPClient::_AppendSlashToBaseIfNecessary()
{
    QString path = _baseUrl.path();
    if (!path.endsWith("/")) {
        path.append("/");
        _baseUrl.setPath(path);
    }
}

FQPRequestSharedPtr
FQPClient::_BuildRequest(const QString& command,
                         const QString& query,
                         const QByteArray& method,
                         const QJsonObject& content, 
                         bool appendTrailingSlash)
{
    QString requestPath = _baseUrl.path();
    QUrl requestUrl = _baseUrl;

    // We've already appended '/' to the base. So see if we need to add the '/'
    // to the command.
    requestPath.append(command);
    if (!command.endsWith("/") && appendTrailingSlash) {
        requestPath.append("/");
    }
    // if ((method == "GET") || (method == "get")) {
    //     // append the content as query parameters.  Umm... TBD?
    //     if (!content.empty()) {
    //         qDebug() << "Get requests with content not implemented";
    //     }
    // }
    
    requestUrl.setPath(requestPath);

    if (!query.isEmpty()) {
        requestUrl.setQuery(query);
    }
    return FQPRequestSharedPtr(new FQPRequest(requestUrl, method, content,
                                              _csrfToken));
}

void
FQPClient::_OnNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessibility)
{
    qDebug() << "FQPClient::_OnNetworkAccessibleChanged(): " << accessibility;
}

 void
 FQPClient::_OnCSRFTokenUpdated(const QByteArray& token)
 {
     _csrfToken = token;
 }

