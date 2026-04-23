// Parse - Locke Werks
// Copyright (c) 2026

#include "ResultPopup.h"

#include <QTextBrowser>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QTextOption>
#include <QScrollBar>

namespace {

QString escape(const QString& s) { return s.toHtmlEscaped().replace('\n', "<br>"); }

QString assistantHtml(const QString& text) {
    return QStringLiteral(
        "<div style='margin:6px 0; color:#e6e6e6;'>%1</div>").arg(escape(text));
}

QString userHtml(const QString& text) {
    return QStringLiteral(
        "<div style='margin:6px 0; text-align:right;'>"
        "<span style='background:#1b2230; padding:4px 8px; border-radius:6px; "
        "color:#e6e6e6;'>%1</span></div>").arg(escape(text));
}

QString errorHtml(const QString& text) {
    return QStringLiteral(
        "<div style='margin:6px 0; color:#ff7a7a;'>%1</div>").arg(escape(text));
}

} // namespace

ResultPopup::ResultPopup(QWidget* parent)
    : QWidget(parent,
              Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint) {
    setAttribute(Qt::WA_ShowWithoutActivating, false);
    setFixedWidth(420);
    setMaximumHeight(520);

    setStyleSheet(QStringLiteral(
        "QWidget { background:#0f1115; color:#e6e6e6; }"
        "QTextBrowser { border:none; background:#0f1115; }"
        "QLineEdit { background:#0f1115; color:#e6e6e6;"
        "  border:1px solid #2a2f3a; border-radius:6px; padding:6px 8px; }"
        "QLabel#status { color:#808a9a; padding:2px 4px; }"));

    m_transcript = new QTextBrowser(this);
    m_transcript->setOpenExternalLinks(true);
    m_transcript->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_transcript->setMinimumHeight(120);

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText(
        QStringLiteral("Ask a followup... (Enter to send, Esc to close)"));
    connect(m_input, &QLineEdit::returnPressed,
            this, &ResultPopup::onSendFollowup);

    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("status"));
    m_status->setText(QString());

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(6);
    lay->addWidget(m_transcript, 1);
    lay->addWidget(m_status);
    lay->addWidget(m_input);
}

void ResultPopup::showNear(const QRect& captureRect) {
    m_historyHtml.clear();
    m_pendingAssistant.clear();
    render();
    m_input->clear();
    setBusy(true);

    adjustSize();
    const QSize sz(width(), qMin(maximumHeight(), sizeHint().height()));
    resize(sz);

    QScreen* screen = QGuiApplication::screenAt(captureRect.center());
    if (!screen) screen = QGuiApplication::primaryScreen();
    const QRect avail = screen ? screen->availableGeometry() : QRect();

    QPoint pos(captureRect.left(), captureRect.bottom() + 8);
    const QRect below(pos, sz);
    if (!avail.contains(below)) {
        pos = QPoint(captureRect.right() - sz.width(),
                     captureRect.top() - sz.height() - 8);
    }
    // Drag it back on-screen in case our math sent it to the shadow realm.
    QRect r(pos, sz);
    if (!avail.isNull()) {
        if (r.right() > avail.right()) r.moveRight(avail.right() - 4);
        if (r.bottom() > avail.bottom()) r.moveBottom(avail.bottom() - 4);
        if (r.left() < avail.left()) r.moveLeft(avail.left() + 4);
        if (r.top() < avail.top()) r.moveTop(avail.top() + 4);
    }
    move(r.topLeft());

    show();
    raise();
    activateWindow();
    m_input->setFocus();
}

void ResultPopup::render() {
    QString html;
    for (const QString& item : m_historyHtml) html += item;
    if (!m_pendingAssistant.isEmpty()) html += assistantHtml(m_pendingAssistant);
    m_transcript->setHtml(html);
    auto* bar = m_transcript->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void ResultPopup::appendAssistantChunk(const QString& delta) {
    m_pendingAssistant.append(delta);
    render();
}

void ResultPopup::finalizeAssistantMessage() {
    if (!m_pendingAssistant.isEmpty()) {
        m_historyHtml.append(assistantHtml(m_pendingAssistant));
        m_pendingAssistant.clear();
        render();
    }
    setBusy(false);
}

void ResultPopup::appendUserMessage(const QString& text) {
    m_historyHtml.append(userHtml(text));
    render();
    setBusy(true);
}

void ResultPopup::showError(const QString& message) {
    m_historyHtml.append(errorHtml(message));
    m_pendingAssistant.clear();
    render();
    setBusy(false);
}

void ResultPopup::setBusy(bool busy) {
    m_status->setText(busy ? QStringLiteral("Thinking…") : QString());
    m_input->setEnabled(!busy);
}

void ResultPopup::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) {
        hide();
        emit dismissed();
        return;
    }
    QWidget::keyPressEvent(e);
}

void ResultPopup::closeEvent(QCloseEvent* e) {
    emit dismissed();
    QWidget::closeEvent(e);
}

void ResultPopup::onSendFollowup() {
    const QString text = m_input->text().trimmed();
    if (text.isEmpty()) return;
    m_input->clear();
    emit followupSubmitted(text);
}
