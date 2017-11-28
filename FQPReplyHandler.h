#ifndef FQPREPLYHANDLER_H
#define FQPREPLYHANDLER_H

#include <QJsonDocument>
#include <QNetworkReply>
#include <QObject>

#include "FQPTypes.h"

FQP_DECLARE_PTRS(QNetworkAccessManager);
FQP_DECLARE_PTRS(FQPRequest);

// Class to hold the closure to be called when the reply is receieved. This
// class doesn't actually call it. Rather, we pass ourselves back to the
// client, with the paramaters to call. To this object, the closure is opaque,
// so we pass the calling responsibility back to the client, along with the
// parameters, as a list of QVariant's.
// 
// So, what do we actually do? We handle the asynchronous receiving of the
// data from the server and the various signals that we receive.
class FQPReplyHandler : public QObject
{
    Q_OBJECT
public:
    // Takes a list of result parameters to pull out of 
    explicit FQPReplyHandler(const QStringList *resultParameters = NULL,
                             QObject *parent = 0);

    ~FQPReplyHandler();

    bool Request(QNetworkAccessManagerSharedPtr accessManager,
                 FQPRequestSharedPtr request);
    
signals:
    void CSRFTokenUpdated(const QByteArray& token);    
    // One of these will be sent upon completion.
    void InterpretedReplyReceived(const QVariantList& parameters);
    void RawReplyReceived(const QJsonDocument& data);

protected:
    enum ResultsFormat {
        NoResults,
        RawResults,
        InterpretedResults,
    };
        
    void _StoreCSRF(const QUrl& baseUrl);
    QJsonDocument _GetJsonFromContent(const QByteArray& content) const;
    void _EmitResults();

protected slots:
    virtual void _OnAuthenticationRequired(QNetworkReply * reply,
                                           QAuthenticator * authenticator);
    virtual void _OnFinished();
    virtual void _OnBytesReceived(qint64 bytesReceived, qint64 bytesTotal);
    virtual void _OnReadyRead();
    virtual void _OnError(QNetworkReply::NetworkError error);

    // Handling SSL errors (Since android gives an error that iOS does not)
    virtual void _OnSslErrors(const QList<QSslError>& errors);

private:
    ResultsFormat _resultsFormat;
    QStringList _resultParameters;
    QNetworkAccessManagerSharedPtr _accessManager;
    FQPRequestSharedPtr _request;
    QNetworkReply *_reply;

    QByteArray _buffer;
    qint64 _bufferSize;
};

#endif // FQPREPLYHANDLER_H
