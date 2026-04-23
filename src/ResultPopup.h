// Parse - Locke Werks
// Copyright (c) 2026

#pragma once
#include <QWidget>
#include <QRect>
#include <QString>
#include <QStringList>

class QTextBrowser;
class QLineEdit;
class QLabel;

class ResultPopup : public QWidget {
    Q_OBJECT
public:
    explicit ResultPopup(QWidget* parent = nullptr);

    void showNear(const QRect& captureRect);

    void appendAssistantChunk(const QString& delta);
    void finalizeAssistantMessage();
    void appendUserMessage(const QString& text);
    void showError(const QString& message);
    void setBusy(bool busy);

signals:
    void followupSubmitted(const QString& text);
    void dismissed();

protected:
    void keyPressEvent(QKeyEvent*) override;
    void closeEvent(QCloseEvent*) override;

private slots:
    void onSendFollowup();

private:
    void render();

    QTextBrowser* m_transcript = nullptr;
    QLineEdit* m_input = nullptr;
    QLabel* m_status = nullptr;

    QStringList m_historyHtml;
    QString m_pendingAssistant;
};
