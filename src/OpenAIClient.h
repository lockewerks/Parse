// Parse - Locke Werks
// Copyright (c) 2026

#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QString>
#include <QJsonArray>

class OpenAIClient : public QObject {
    Q_OBJECT
public:
    explicit OpenAIClient(QObject* parent = nullptr);

    void setApiKey(const QString& key);
    void startConversation(const QByteArray& pngBase64);
    void sendFollowup(const QString& userText);
    void reset();

signals:
    void chunkReceived(const QString& delta);
    void completed();
    void errorOccurred(const QString& message);

private slots:
    void onReadyRead();
    void onFinished();

private:
    void send();

    QNetworkAccessManager m_net;
    QNetworkReply* m_reply = nullptr;
    QString m_apiKey;
    QJsonArray m_messages;
    QString m_assistantAccumulator;
    QByteArray m_sseBuffer;
};
