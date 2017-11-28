#include "FQPRequest.h"

#include "FQPClient.h"

#include <QJsonDocument>
#include <QHttpMultiPart>

FQPRequest::FQPRequest(const QUrl& url,
                       const QByteArray& method,
                       const QJsonObject& content,
                       const QByteArray& csrfToken) :
    _method(method)
{
    _request = QNetworkRequest(url);
    // We do set this in the content, so it may not be necessary to do it
    // twice.
    _request.setHeader(QNetworkRequest::ContentTypeHeader,
                       "application/json");
    if (csrfToken.length() > 0) {
        _request.setRawHeader(FQPClient::CSRFHeaderName, csrfToken);
    }

    if (!content.isEmpty()) {
        QJsonDocument doc;
        doc.setObject(content);
        _content = doc.toJson(QJsonDocument::Compact);
    }
}

FQPRequest::~FQPRequest()
{
    //
}

QNetworkRequest
FQPRequest::GetRequest() const
{
    return _request;
}

QByteArray
FQPRequest::GetMethod() const{
    return _method;
}

QByteArray
FQPRequest::GetContent() const
{
    return _content;
}
