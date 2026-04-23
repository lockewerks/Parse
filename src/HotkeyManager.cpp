// Parse - Locke Werks
// Copyright (c) 2026

#include "HotkeyManager.h"

#include <QCoreApplication>

#include <Windows.h>

HotkeyManager::HotkeyManager(QObject* parent) : QObject(parent) {
    QCoreApplication::instance()->installNativeEventFilter(this);
}

HotkeyManager::~HotkeyManager() {
    unregisterHotkey();
    if (auto* app = QCoreApplication::instance()) {
        app->removeNativeEventFilter(this);
    }
}

bool HotkeyManager::registerHotkey(unsigned int mods, unsigned int vk) {
    unregisterHotkey();
    // Passing nullptr for hWnd makes Windows route WM_HOTKEY to the thread
    // message queue instead of a specific window. Qt slurps it up via the
    // native event filter below. If you "fix" this to use a real HWND,
    // congratulations, you have just introduced a bug.
    if (!::RegisterHotKey(nullptr, m_id, mods, vk)) {
        return false;
    }
    m_registered = true;
    return true;
}

void HotkeyManager::unregisterHotkey() {
    if (m_registered) {
        ::UnregisterHotKey(nullptr, m_id);
        m_registered = false;
    }
}

bool HotkeyManager::nativeEventFilter(const QByteArray& eventType,
                                      void* message, qintptr* /*result*/) {
    if (eventType != "windows_generic_MSG") {
        return false;
    }
    auto* msg = static_cast<MSG*>(message);
    if (msg->message == WM_HOTKEY && static_cast<int>(msg->wParam) == m_id) {
        emit triggered();
        return true;
    }
    return false;
}
