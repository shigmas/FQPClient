#ifndef FQPREQUEST_H
#define FQPREQUEST_H

#include "FQPTypes.h"

#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>

FQP_DECLARE_PTRS(QHttpMultiPart)

class FQPRequest
{
public:
    explicit FQPRequest(const QUrl& url,
                        const QByteArray& method,
                        const QJsonObject& content,
                        const QByteArray& csrfToken);

    virtual ~FQPRequest();

    QNetworkRequest GetRequest() const;
    QByteArray GetMethod() const;
    QByteArray GetContent() const;

private:
    QNetworkRequest _request;
    QByteArray _method;
    QByteArray _content;
};

#endif // FQPREQUEST_H
