// Parse - Locke Werks
// Copyright (c) 2026

#include <QApplication>
#include <QStyleFactory>

#include <Windows.h>

#include "SingleInstance.h"
#include "AppController.h"

int main(int argc, char* argv[]) {
    // Must run before QApplication wakes up and forms opinions about DPI.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Parse"));
    QApplication::setOrganizationName(QStringLiteral("Locke Werks"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    SingleInstance guard;
    if (!guard.acquire()) {
        return 0;
    }

    AppController controller;
    if (!controller.initialize()) {
        return 1;
    }

    return app.exec();
}
