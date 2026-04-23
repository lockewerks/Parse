// Parse - Locke Werks
// Copyright (c) 2026

#pragma once
#include <QObject>
#include <QRect>

class QSystemTrayIcon;
class QMenu;
class HotkeyManager;
class RegionSelector;
class OpenAIClient;
class ResultPopup;

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr);
    bool initialize();

private slots:
    void onHotkeyTriggered();
    void onRegionSelected(const QRect& rect);
    void onRegionCancelled();
    void onAssistantChunk(const QString& delta);
    void onAssistantCompleted();
    void onAssistantError(const QString& msg);
    void onFollowupSubmitted(const QString& text);
    void onPopupDismissed();
    void onTrayActivated(int reason);
    void onQuit();

private:
    void buildTrayMenu();

    QSystemTrayIcon* m_tray = nullptr;
    QMenu* m_trayMenu = nullptr;
    HotkeyManager* m_hotkey = nullptr;
    RegionSelector* m_selector = nullptr;
    OpenAIClient* m_openai = nullptr;
    ResultPopup* m_popup = nullptr;

    QRect m_lastCaptureRect;
    bool m_inProgress = false;
};
