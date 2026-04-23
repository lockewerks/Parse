// Parse - Locke Werks
// Copyright (c) 2026

#include "SingleInstance.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QCryptographicHash>

#include <Windows.h>
#include <sddl.h>

namespace {

QString currentUserSidHash() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return QStringLiteral("nosid");
    }

    DWORD needed = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &needed);
    QByteArray buf(static_cast<int>(needed), 0);
    if (!GetTokenInformation(token, TokenUser, buf.data(), needed, &needed)) {
        CloseHandle(token);
        return QStringLiteral("nosid");
    }
    const TOKEN_USER* tu = reinterpret_cast<const TOKEN_USER*>(buf.constData());

    LPWSTR sidStr = nullptr;
    QString sidQ = QStringLiteral("nosid");
    if (ConvertSidToStringSidW(tu->User.Sid, &sidStr)) {
        sidQ = QString::fromWCharArray(sidStr);
        LocalFree(sidStr);
    }
    CloseHandle(token);

    const QByteArray digest = QCryptographicHash::hash(
        sidQ.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QString::fromLatin1(digest.left(16));
}

} // namespace

SingleInstance::SingleInstance(QObject* parent)
    : QObject(parent)
    , m_key(QStringLiteral("parse-singleton-") + currentUserSidHash()) {}

bool SingleInstance::acquire() {
    // Knock on the named pipe. If somebody answers, we are the second copy
    // and should politely show ourselves out.
    QLocalSocket probe;
    probe.connectToServer(m_key);
    if (probe.waitForConnected(500)) {
        probe.disconnectFromServer();
        return false;
    }

    // Nobody home. Sweep up whatever the previous crash left behind and
    // claim the name for ourselves.
    QLocalServer::removeServer(m_key);
    m_server = new QLocalServer(this);
    if (!m_server->listen(m_key)) {
        return false;
    }
    return true;
}
