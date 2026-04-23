// Parse - Locke Werks
// Copyright (c) 2026

#include "AppController.h"

#include "HotkeyManager.h"
#include "RegionSelector.h"
#include "Capturer.h"
#include "OpenAIClient.h"
#include "ResultPopup.h"

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QIcon>
#include <QImage>
#include <QDir>
#include <QDateTime>
#include <QDebug>

#include <Windows.h>

AppController::AppController(QObject* parent) : QObject(parent) {}

bool AppController::initialize() {
    const QByteArray key = qgetenv("OPENAI_API_KEY");
    if (key.isEmpty()) {
        QMessageBox::critical(nullptr, QStringLiteral("Parse"),
            QStringLiteral(
                "OPENAI_API_KEY is not set.\n\n"
                "Set it in your environment and relaunch, e.g.:\n"
                "    setx OPENAI_API_KEY sk-...\n"
                "(then open a new terminal.)"));
        return false;
    }

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, QStringLiteral("Parse"),
            QStringLiteral("No system tray available."));
        return false;
    }

    m_openai = new OpenAIClient(this);
    m_openai->setApiKey(QString::fromUtf8(key));

    m_selector = new RegionSelector();
    m_popup = new ResultPopup();
    m_hotkey = new HotkeyManager(this);

    connect(m_hotkey, &HotkeyManager::triggered,
            this, &AppController::onHotkeyTriggered);
    connect(m_selector, &RegionSelector::regionSelected,
            this, &AppController::onRegionSelected);
    connect(m_selector, &RegionSelector::cancelled,
            this, &AppController::onRegionCancelled);
    connect(m_openai, &OpenAIClient::chunkReceived,
            this, &AppController::onAssistantChunk);
    connect(m_openai, &OpenAIClient::completed,
            this, &AppController::onAssistantCompleted);
    connect(m_openai, &OpenAIClient::errorOccurred,
            this, &AppController::onAssistantError);
    connect(m_popup, &ResultPopup::followupSubmitted,
            this, &AppController::onFollowupSubmitted);
    connect(m_popup, &ResultPopup::dismissed,
            this, &AppController::onPopupDismissed);

    if (m_hotkey->registerHotkey(MOD_CONTROL | MOD_SHIFT, VK_OEM_2)) {
        qDebug() << "Parse: hotkey bound to Ctrl+Shift+/";
    } else if (m_hotkey->registerHotkey(
                   MOD_CONTROL | MOD_SHIFT | MOD_ALT, 'P')) {
        qDebug() << "Parse: primary hotkey taken, bound to Ctrl+Shift+Alt+P";
    } else {
        QMessageBox::warning(nullptr, QStringLiteral("Parse"),
            QStringLiteral("Could not register a global hotkey. "
                           "Use the tray menu to capture."));
    }

    buildTrayMenu();
    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(QIcon(QStringLiteral(":/icons/tray.png")));
    m_tray->setToolTip(QStringLiteral("Parse"));
    m_tray->setContextMenu(m_trayMenu);
    connect(m_tray, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason r) {
                onTrayActivated(static_cast<int>(r));
            });
    m_tray->show();

    return true;
}

void AppController::buildTrayMenu() {
    m_trayMenu = new QMenu();

    auto* capture = m_trayMenu->addAction(
        QStringLiteral("Capture (Ctrl+Shift+/)"));
    connect(capture, &QAction::triggered,
            this, &AppController::onHotkeyTriggered);

    m_trayMenu->addSeparator();

    auto* about = m_trayMenu->addAction(QStringLiteral("About Parse"));
    connect(about, &QAction::triggered, this, [] {
        QMessageBox::information(nullptr, QStringLiteral("Parse"),
            QStringLiteral("Parse 1.0.0\nLocke Werks"));
    });

    auto* quit = m_trayMenu->addAction(QStringLiteral("Quit"));
    connect(quit, &QAction::triggered, this, &AppController::onQuit);
}

void AppController::onHotkeyTriggered() {
    if (m_inProgress) return;
    m_inProgress = true;
    m_selector->start();
}

void AppController::onRegionCancelled() {
    m_inProgress = false;
}

void AppController::onRegionSelected(const QRect& rect) {
    m_lastCaptureRect = rect;
    const QImage img = Capturer::capture(rect);
    if (img.isNull()) {
        m_inProgress = false;
        QMessageBox::warning(nullptr, QStringLiteral("Parse"),
            QStringLiteral("Failed to capture region."));
        return;
    }
    const QString inboxDir = QDir::homePath()
        + QStringLiteral("/projects/Reliquary/inbox/files");
    QDir().mkpath(inboxDir);
    const QString inboxFile = inboxDir + QStringLiteral("/parse-")
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmsszzz"))
        + QStringLiteral(".png");
    if (!img.save(inboxFile, "PNG")) {
        qDebug() << "Parse: failed to write" << inboxFile;
    }

    const QByteArray b64 = Capturer::toPngBase64(img);
    if (b64.isEmpty()) {
        m_inProgress = false;
        QMessageBox::warning(nullptr, QStringLiteral("Parse"),
            QStringLiteral("Failed to encode capture."));
        return;
    }

    m_popup->showNear(rect);
    m_openai->startConversation(b64);
}

void AppController::onAssistantChunk(const QString& delta) {
    m_popup->appendAssistantChunk(delta);
}

void AppController::onAssistantCompleted() {
    m_popup->finalizeAssistantMessage();
    m_inProgress = false;
}

void AppController::onAssistantError(const QString& msg) {
    m_popup->showError(msg);
    m_inProgress = false;
}

void AppController::onFollowupSubmitted(const QString& text) {
    m_popup->appendUserMessage(text);
    m_openai->sendFollowup(text);
}

void AppController::onPopupDismissed() {
    m_openai->reset();
    m_inProgress = false;
}

void AppController::onTrayActivated(int reason) {
    if (reason == QSystemTrayIcon::DoubleClick) {
        onHotkeyTriggered();
    }
}

void AppController::onQuit() {
    if (m_popup) m_popup->hide();
    if (m_selector) m_selector->hide();
    QApplication::quit();
}
