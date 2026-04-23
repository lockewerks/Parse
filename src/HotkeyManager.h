// Parse - Locke Werks
// Copyright (c) 2026

#pragma once
#include <QObject>
#include <QAbstractNativeEventFilter>

class HotkeyManager : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager() override;

    bool registerHotkey(unsigned int mods, unsigned int vk);
    void unregisterHotkey();

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void triggered();

private:
    int m_id = 1;
    bool m_registered = false;
};
