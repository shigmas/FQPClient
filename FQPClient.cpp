#include "FQPClient.h"

#include "FQPRequest.h"

#include <QNetworkAccessManager>

const QByteArray FQPClient::CSRFCookieName = "csrftoken";
const QByteArray FQPClient::CSRFHeaderName = "X-CSRFTOKEN";

FQPClient::FQPClient(const QString& host,
                     const QString& basePath,
                     const QString& scheme,
                     QObject *parent)
    : QObject(parent),
    _accessManager(new QNetworkAccessManager())
{
    // We could just call the other constructor, but we need to do some
    // verification of the arguments
    if ((scheme != "http") || (scheme != "https")) {
        throw FQPUrlException("Invalid scheme");
    }

    if (basePath.at(0) == '/') {
        _baseUrl = scheme + "://" + host + basePath;
    } else {
        _baseUrl = scheme + "://" + host + "/" + basePath;
    }
    _AppendSlashToBaseIfNecessary();
}


FQPClient::FQPClient(const QUrl& baseUrl, QObject *parent) :
    QObject(parent),
    _baseUrl(baseUrl),
    _accessManager(new QNetworkAccessManager())
{
    _AppendSlashToBaseIfNecessary();
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
                         const QByteArray& method,
                         const QJsonObject& content)
{
    QString requestPath = _baseUrl.path();
    QUrl requestUrl = _baseUrl;

    // We've already appended '/' to the base. So see if we need to add the '/'
    // to the command.
    requestPath.append(command);
    if (!command.endsWith("/")) {
        requestPath.append("/");
    }
    if ((method == "GET") || (method == "get")) {
        // append the content as query parameters.  Umm... TBD?
        if (!content.empty()) {
            qDebug() << "Get requests with content not implemented";
        }
    }
    
    requestUrl.setPath(requestPath);

    return FQPRequestSharedPtr(new FQPRequest(requestUrl, method, content,
                                              _csrfToken));
}
