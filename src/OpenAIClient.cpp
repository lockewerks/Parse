// Parse - Locke Werks
// Copyright (c) 2026

#include "OpenAIClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>

namespace {

constexpr const char* kEndpoint = "https://api.openai.com/v1/chat/completions";
constexpr const char* kModel = "gpt-4o";

const QString& systemPrompt() {
    static const QString s = QStringLiteral(
        "You are Parse, a visual interpretation assistant. The user has captured "
        "a region of their screen and wants to understand what they are looking at. "
        "Analyze the image:\n\n"
        "- If it is primarily text (chat, email, document, code, error message), "
        "explain what the author is actually communicating in plain language. "
        "Decode acronyms, jargon, corporate-speak, or cryptic references using "
        "context. Be direct and concrete.\n"
        "- If it is a user interface, describe what application or screen it is "
        "and what the user is likely trying to do.\n"
        "- If it is a diagram, chart, or mixed content, describe what it represents "
        "and the key takeaways.\n\n"
        "Rules:\n"
        "- Be concise. No preamble like \"I can see\" or \"This appears to be\". "
        "Just answer.\n"
        "- Do not hedge with phrases like \"it seems\" or \"possibly\" unless "
        "genuinely uncertain.\n"
        "- If asked a followup, answer it directly using the original image as "
        "context.\n"
        "- No emoji. No markdown headers. Short paragraphs.");
    return s;
}

QJsonObject textPart(const QString& t) {
    QJsonObject o;
    o.insert("type", "text");
    o.insert("text", t);
    return o;
}

QJsonObject imagePart(const QByteArray& pngBase64) {
    QJsonObject url;
    url.insert("url", QStringLiteral("data:image/png;base64,")
                          + QString::fromLatin1(pngBase64));
    QJsonObject o;
    o.insert("type", "image_url");
    o.insert("image_url", url);
    return o;
}

} // namespace

OpenAIClient::OpenAIClient(QObject* parent) : QObject(parent) {}

void OpenAIClient::setApiKey(const QString& key) { m_apiKey = key; }

void OpenAIClient::reset() {
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_messages = QJsonArray();
    m_assistantAccumulator.clear();
    m_sseBuffer.clear();
}

void OpenAIClient::startConversation(const QByteArray& pngBase64) {
    reset();

    QJsonObject sys;
    sys.insert("role", "system");
    sys.insert("content", systemPrompt());
    m_messages.append(sys);

    QJsonArray content;
    content.append(textPart(QStringLiteral("What is this?")));
    content.append(imagePart(pngBase64));
    QJsonObject user;
    user.insert("role", "user");
    user.insert("content", content);
    m_messages.append(user);

    send();
}

void OpenAIClient::sendFollowup(const QString& userText) {
    if (!m_assistantAccumulator.isEmpty()) {
        QJsonObject a;
        a.insert("role", "assistant");
        a.insert("content", m_assistantAccumulator);
        m_messages.append(a);
        m_assistantAccumulator.clear();
    }
    QJsonObject u;
    u.insert("role", "user");
    u.insert("content", userText);
    m_messages.append(u);

    send();
}

void OpenAIClient::send() {
    if (m_apiKey.isEmpty()) {
        emit errorOccurred(QStringLiteral("No API key set."));
        return;
    }
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_sseBuffer.clear();

    QJsonObject body;
    body.insert("model", QString::fromLatin1(kModel));
    body.insert("stream", true);
    body.insert("messages", m_messages);

    QNetworkRequest req{ QUrl(QString::fromLatin1(kEndpoint)) };
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     "Bearer " + m_apiKey.toUtf8());
    req.setRawHeader("Accept", "text/event-stream");

    m_reply = m_net.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::readyRead,
            this, &OpenAIClient::onReadyRead);
    connect(m_reply, &QNetworkReply::finished,
            this, &OpenAIClient::onFinished);
}

// Server-sent events: frames separated by \n\n, each interesting line
// prefixed with "data: ", and [DONE] at the end as a sign-off. We buffer
// bytes, slice on blank lines, and parse each JSON chunk by hand because
// Qt still doesn't ship an SSE reader in the year of our Lord 2026.
void OpenAIClient::onReadyRead() {
    if (!m_reply) return;
    m_sseBuffer.append(m_reply->readAll());

    while (true) {
        const int term = m_sseBuffer.indexOf("\n\n");
        if (term < 0) break;

        const QByteArray event = m_sseBuffer.left(term);
        m_sseBuffer.remove(0, term + 2);

        for (const QByteArray& line : event.split('\n')) {
            if (!line.startsWith("data:")) continue;
            QByteArray payload = line.mid(5);
            if (payload.startsWith(' ')) payload.remove(0, 1);

            if (payload == "[DONE]") {
                emit completed();
                continue;
            }

            QJsonParseError err{};
            const auto doc = QJsonDocument::fromJson(payload, &err);
            if (err.error != QJsonParseError::NoError) continue;

            const auto choices = doc.object().value("choices").toArray();
            if (choices.isEmpty()) continue;
            const auto delta = choices.first().toObject().value("delta").toObject();
            const QString content = delta.value("content").toString();
            if (content.isEmpty()) continue;

            m_assistantAccumulator.append(content);
            emit chunkReceived(content);
        }
    }
}

void OpenAIClient::onFinished() {
    if (!m_reply) return;

    const int status = m_reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto netErr = m_reply->error();

    m_reply->deleteLater();
    m_reply = nullptr;

    if (netErr != QNetworkReply::NoError) {
        if (status == 401) {
            emit errorOccurred(QStringLiteral(
                "API key rejected. Update OPENAI_API_KEY and relaunch."));
        } else if (status == 429) {
            emit errorOccurred(QStringLiteral(
                "Rate limited. Try again in a moment."));
        } else {
            emit errorOccurred(QStringLiteral("Request failed (HTTP %1).")
                                   .arg(status));
        }
        return;
    }

    if (!m_assistantAccumulator.isEmpty()) {
        emit completed();
    }
}
