#ifndef FQPCLIENT_H
#define FQPCLIENT_H

#include <QObject>

#include "FQPReplyHandler.h"
#include "FQPRequest.h"
#include "FQPTypes.h"

// Network stuff
#include <QString>
#include <QUrl>

#include <QVector>

// JSON stuff
#include <QJsonObject>
#include <QStringList>

#include <functional>

FQP_DECLARE_PTRS(QNetworkAccessManager);
FQP_DECLARE_PTRS(FQPReplyHandler);
FQP_DECLARE_PTRS(FQPRequest);

class FQPClient : public QObject
{
    Q_OBJECT

public:
    static const QByteArray CSRFCookieName;
    static const QByteArray CSRFHeaderName;

    // Constructs a client to communicate with the host (and port) and
    // the initial part of the URL specified by baseUrl. The scheme may
    // by regular http or https.
    explicit FQPClient(const QString& host,
                       const QString& basePath,
                       const QString& scheme,
                       QObject *parent = 0);

    // Constructs a client to communicate with the base URL.
    explicit FQPClient(const QUrl& baseUrl, QObject *parent = 0);

    // Makes a request with the command to be appended to the baseUrl.
    // The method is one of the HTTP methods.
    // parameters is the JSON parameters to send as part of the request.
    // resultParameters is the list of parameters to pass to the handler.
    // Some special values for the parameters: NULL for a handler that
    // takes no parameters, and an empty list to pass the raw results to
    // the handler, doing no handling of the results.
    // XXX Quite hacky, but we have to have a version for 1..n arguments.
    void FetchRaw(const QString& command,
                  const QByteArray& method,
                  const QJsonObject& parameters,
                  std::function<void (const QJsonDocument&)> handler) {
        FQPRequestSharedPtr request = _BuildRequest(command, method,
                                                    parameters);
        qDebug() << "raw URL: " << request->GetRequest().url();
        QStringList rawParams;
        FQPReplyHandler *reply = new FQPReplyHandler(&rawParams);
        _handlers.append(FQPReplyHandlerSharedPtr(reply));
        // We need to keep a reference to the reply around. We need some kind of
        // cleanup in the closure to get rid of any resources that were created
        // in this function. We don't care about the parameter passed in from the
        // signal.
        connect(reply, &FQPReplyHandler::RawReplyReceived,
                [request, reply, handler](const QJsonDocument& jsonDoc) {
                    qDebug() << "Request completed. calling handler";
                    if (jsonDoc.isEmpty()) {
                        qDebug() << "Is empty";
                    }
                    handler(jsonDoc);  });
        reply->Request(_accessManager, request);
    }

    void Fetch(const QString& command,
               const QByteArray& method,
               const QJsonObject& parameters,
               std::function<void ()> handler) {
        FQPRequestSharedPtr request = _BuildRequest(command, method,
                                                    parameters);
        qDebug() << "URL: " << request->GetRequest().url();
        FQPReplyHandler *reply = new FQPReplyHandler();
        _handlers.append(FQPReplyHandlerSharedPtr(reply));
        // We need to keep a reference to the reply around. We need some kind of
        // cleanup in the closure to get rid of any resources that were created
        // in this function. We don't care about the parameter passed in from the
        // signal.
        connect(reply, &FQPReplyHandler::InterpretedReplyReceived,
                [request, reply, handler]() {
                    qDebug() << "Request completed. calling handler";
                    handler();  });
        reply->Request(_accessManager, request);
    }

    template <typename S>
    void Fetch(const QString& command,
               const QByteArray& method,
               const QJsonObject& parameters,
               const QStringList* resultParameters,
               std::function<void (S)> handler) {
        FQPRequestSharedPtr request = _BuildRequest(command, method,
                                                    parameters);
        qDebug() << "(One arg version)URL: " << request->GetRequest().url();
        FQPReplyHandler *reply = new FQPReplyHandler(resultParameters);
        _handlers.append(FQPReplyHandlerSharedPtr(reply));
        connect(reply, &FQPReplyHandler::InterpretedReplyReceived,
                [request, reply, handler] (const QVariantList& results) {
                    // Only one element
                    qDebug() << "Request completed. handling "
                             << results.size();
                    S val = FQPType_GetValueFromVariant<S>(results[0]);
                    handler(val);
                });
        reply->Request(_accessManager, request);
    }

    template <typename S,
              typename T>
    void Fetch(const QString& command,
               const QByteArray& method,
               const QJsonObject& parameters,
               const QStringList* resultParameters,
               std::function<void (S, T)> handler) {
        FQPRequestSharedPtr request = _BuildRequest(command, method,
                                                    parameters);
        qDebug() << "(Two arg version)URL: " << request->GetRequest().url();
        FQPReplyHandler *reply = new FQPReplyHandler(resultParameters);
        _handlers.append(FQPReplyHandlerSharedPtr(reply));
        connect(reply, &FQPReplyHandler::InterpretedReplyReceived,
                [request, reply, handler] (const QVariantList& results) {
                    // Only one element
                    qDebug() << "Request completed. handling "
                             << results.size();
                    S v0 = FQPType_GetValueFromVariant<S>(results[0]);
                    T v1 = FQPType_GetValueFromVariant<T>(results[1]);
                    handler(v0, v1);
                });
        reply->Request(_accessManager, request);
    }

    template <typename S,
              typename T,
              typename U>
    void Fetch(const QString& command,
               const QByteArray& method,
               const QJsonObject& parameters,
               const QStringList* resultParameters,
               std::function<void (S, T, U)> handler) {
        FQPRequestSharedPtr request = _BuildRequest(command, method,
                                                    parameters);
        qDebug() << "(three arg version)URL: " << request->GetRequest().url();
        FQPReplyHandler *reply = new FQPReplyHandler(resultParameters);
        _handlers.append(FQPReplyHandlerSharedPtr(reply));
        connect(reply, &FQPReplyHandler::InterpretedReplyReceived,
                [request, reply, handler] (const QVariantList& results) {
                    // Only one element
                    qDebug() << "Request completed. handling "
                             << results.size();
                    S v0 = FQPType_GetValueFromVariant<S>(results[0]);
                    T v1 = FQPType_GetValueFromVariant<T>(results[1]);
                    U v2 = FQPType_GetValueFromVariant<U>(results[2]);
                    handler(v0, v1, v2);
                });
        reply->Request(_accessManager, request);
    }

signals:

protected slots:

protected:
    void _AppendSlashToBaseIfNecessary();

    // Build the request with the command and method and content. The request
    // is is essentially the URL. If it's a GET, the URL contains the query
    // parameters. Otherwise, the request also contains the Http Multipart data
    // containing the JSON body.
    FQPRequestSharedPtr _BuildRequest(const QString& command,
                                      const QByteArray& method,
                                      const QJsonObject& content);

private:
    QUrl _baseUrl;
    QNetworkAccessManagerSharedPtr _accessManager;
    QByteArray _csrfToken;
    QVector<FQPReplyHandlerSharedPtr> _handlers;
};

#endif // FQPCLIENT_H
