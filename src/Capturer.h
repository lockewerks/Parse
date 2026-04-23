// Parse - Locke Werks
// Copyright (c) 2026

#pragma once
#include <QImage>
#include <QRect>
#include <QByteArray>

class Capturer {
public:
    static QImage capture(const QRect& virtualRect);
    static QByteArray toPngBase64(const QImage& img);
};
