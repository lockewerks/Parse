// Parse - Locke Werks
// Copyright (c) 2026

#pragma once
#include <QObject>

class QLocalServer;

class SingleInstance : public QObject {
    Q_OBJECT
public:
    explicit SingleInstance(QObject* parent = nullptr);
    bool acquire();

private:
    QLocalServer* m_server = nullptr;
    QString m_key;
};
