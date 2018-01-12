#ifndef FQPCLIENT_H
#define FQPCLIENT_H

#include <QObject>

#include "FQPReplyHandler.h"
#include "FQPRequest.h"
#include "FQPTypes.h"

#include <QThread>
// Network stuff
#include <QString>
#include <QUrl>

#include <QQueue>
#include <QVector>

// JSON stuff
#include <QJsonObject>
#include <QStringList>

#include <functional>

FQP_DECLARE_PTRS(QNetworkAccessManager)
FQP_DECLARE_PTRS(QEventLoop)
FQP_DECLARE_PTRS(FQPReplyHandler)
FQP_DECLARE_PTRS(FQPRequest)

// Inherited from thread, but we don't have to run as a thread.
class FQPClient : public QThread
{
    Q_OBJECT

public:
    static const QByteArray CSRFCookieName;
    static const QByteArray CSRFHeaderName;

    // Only necessary to call if we want to run this in a separate thread.
    // XXX - This seems like it works in the simulator, but not iOS.
    virtual void run() override;

    // Constructs a client to communicate with the base URL.
    explicit FQPClient(const QUrl& baseUrl, QObject *parent = 0);

    bool IsNetworkAccessible() const;

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
        FQPReplyHandlerSharedPtr reply =
            FQPReplyHandlerSharedPtr(new FQPReplyHandler(_accessManager,
                                                         request, &rawParams));
        // We need to keep a reference to the reply around. We need some kind of
        // cleanup in the closure to get rid of any resources that were created
        // in this function. We don't care about the parameter passed in from the
        // signal.
        connect(reply.get(), &FQPReplyHandler::RawReplyReceived,
                [request, reply, handler](QNetworkReply::NetworkError error,
                                          const QJsonDocument& jsonDoc) {
                    if (error != QNetworkReply::NoError) {
                        qDebug() << "error: " << error;
                    }
                    if (jsonDoc.isEmpty()) {
                        qDebug() << "Is empty";
                    }
                    handler(jsonDoc);  });
        _QueueRequest(request, reply);
    }

    void Fetch(const QString& command,
               const QByteArray& method,
               const QJsonObject& parameters,
               std::function<void ()> handler) {
        FQPRequestSharedPtr request = _BuildRequest(command, method,
                                                    parameters);
        qDebug() << "URL: " << request->GetRequest().url();
        FQPReplyHandlerSharedPtr reply =
            FQPReplyHandlerSharedPtr(new FQPReplyHandler(_accessManager,
                                                         request));
        // We need to keep a reference to the reply around. We need some kind of
        // cleanup in the closure to get rid of any resources that were created
        // in this function. We don't care about the parameter passed in from the
        // signal.
        connect(reply.get(), &FQPReplyHandler::InterpretedReplyReceived,
                [request, reply, handler](QNetworkReply::NetworkError error) {
                    if (error != QNetworkReply::NoError) {
                        qDebug() << "error: " << error;
                    }
                    handler();  });
        _QueueRequest(request, reply);
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
        FQPReplyHandlerSharedPtr reply =
            FQPReplyHandlerSharedPtr(new FQPReplyHandler(_accessManager,
                                                         request,
                                                         resultParameters));
        connect(reply.get(), &FQPReplyHandler::InterpretedReplyReceived,
                [request, reply, handler] (QNetworkReply::NetworkError error,
                                           const QVariantList& results) {
                    if (error != QNetworkReply::NoError) {
                        qDebug() << "error: " << error;
                    }
                    // Only one element
                    S val = FQPType_GetValueFromVariant<S>(results[0]);
                    handler(val);
                });
        _QueueRequest(request, reply);
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
        FQPReplyHandlerSharedPtr reply =
            FQPReplyHandlerSharedPtr(new FQPReplyHandler(_accessManager,
                                                         request,
                                                         resultParameters));
        connect(reply.get(), &FQPReplyHandler::InterpretedReplyReceived,
                [request, reply, handler] (QNetworkReply::NetworkError error,
                                           const QVariantList& results) {
                    if (error != QNetworkReply::NoError) {
                        qDebug() << "error: " << error;
                    }
                    // Only one element
                    S v0 = FQPType_GetValueFromVariant<S>(results[0]);
                    T v1 = FQPType_GetValueFromVariant<T>(results[1]);
                    handler(v0, v1);
                });
        _QueueRequest(request, reply);
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
        FQPReplyHandlerSharedPtr reply =
            FQPReplyHandlerSharedPtr(new FQPReplyHandler(_accessManager,
                                                         request,
                                                         resultParameters));
        connect(reply.get(), &FQPReplyHandler::InterpretedReplyReceived,
                [request, handler] (QNetworkReply::NetworkError error,
                                    const QVariantList& results) {
                    if (error != QNetworkReply::NoError) {
                        qDebug() << "error: " << error;
                    }
                    // Only one element
                    S v0 = FQPType_GetValueFromVariant<S>(results[0]);
                    T v1 = FQPType_GetValueFromVariant<T>(results[1]);
                    U v2 = FQPType_GetValueFromVariant<U>(results[2]);
                    handler(v0, v1, v2);
                });
        _QueueRequest(request, reply);
    }

    template <typename S,
              typename T,
              typename U,
              typename V>
    void Fetch(const QString& command,
               const QByteArray& method,
               const QJsonObject& parameters,
               const QStringList* resultParameters,
               std::function<void (S, T, U, V)> handler) {
        FQPRequestSharedPtr request = _BuildRequest(command, method,
                                                    parameters);
        qDebug() << "(four arg version)URL: " << request->GetRequest().url();
        FQPReplyHandlerSharedPtr reply =
            FQPReplyHandlerSharedPtr(new FQPReplyHandler(_accessManager,
                                                         request,
                                                         resultParameters));
        connect(reply.get(), &FQPReplyHandler::InterpretedReplyReceived,
                [request, reply, handler] (QNetworkReply::NetworkError error,
                                           const QVariantList& results) {
                    if (error != QNetworkReply::NoError) {
                        qDebug() << "error: " << error;
                    }
                    // Only one element
                    S v0 = FQPType_GetValueFromVariant<S>(results[0]);
                    T v1 = FQPType_GetValueFromVariant<T>(results[1]);
                    U v2 = FQPType_GetValueFromVariant<U>(results[2]);
                    V v3 = FQPType_GetValueFromVariant<U>(results[3]);
                    handler(v0, v1, v2, v3);
                });
        _QueueRequest(request, reply);
    }

signals:

protected:
    QNetworkAccessManagerSharedPtr _InitAccessManager();

    void _QueueRequest(const FQPRequestSharedPtr& request,
                       const FQPReplyHandlerSharedPtr& reply);


    void _AppendSlashToBaseIfNecessary();

    // Build the request with the command and method and content. The request
    // is is essentially the URL. If it's a GET, the URL contains the query
    // parameters. Otherwise, the request also contains the Http Multipart data
    // containing the JSON body.
    FQPRequestSharedPtr _BuildRequest(const QString& command,
                                      const QByteArray& method,
                                      const QJsonObject& content);

protected slots:
    virtual void _OnNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessibility);

    virtual void _OnCSRFTokenUpdated(const QByteArray& token);
    
private:
    QUrl _baseUrl;
    QNetworkAccessManagerSharedPtr _accessManager;
    QByteArray _csrfToken;

    // Only valid (and needed) when we run as a separate thread. Thus, it's
    // the flag for running as a thread.
    QEventLoopSharedPtr _eventLoop;

    QQueue<std::pair<FQPRequestSharedPtr,
                     FQPReplyHandlerSharedPtr> > _requestQueue;
};

#endif // FQPCLIENT_H

