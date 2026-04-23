// Parse - Locke Werks
// Copyright (c) 2026

#include "Capturer.h"

#include <QBuffer>
#include <QGuiApplication>
#include <QScreen>
#include <cmath>

#include <Windows.h>

namespace {

struct ScreenDc {
    HDC h = ::GetDC(nullptr);
    ~ScreenDc() { if (h) ::ReleaseDC(nullptr, h); }
};

struct MemDc {
    HDC h = nullptr;
    explicit MemDc(HDC ref) : h(::CreateCompatibleDC(ref)) {}
    ~MemDc() { if (h) ::DeleteDC(h); }
};

struct Bmp {
    HBITMAP h = nullptr;
    ~Bmp() { if (h) ::DeleteObject(h); }
};

} // namespace

// GDI screen capture, the same trick your grandparents used. Works everywhere
// except on hardware-overlay surfaces (protected video, some Chromium windows),
// which come back as cheerful black rectangles. The proper fix is the Windows
// Graphics Capture API. That's a tomorrow problem.
QImage Capturer::capture(const QRect& virtualRect) {
    if (virtualRect.isEmpty()) return {};

    // Qt measures in logical pixels. BitBlt measures in physical pixels.
    // Qt and Windows refuse to sit at the same table, so we translate via
    // the target screen's DPR. Mixed-DPI setups where a single rect spans
    // screens at different scale factors will render slightly drunk.
    const QScreen* target = QGuiApplication::screenAt(virtualRect.center());
    if (!target) target = QGuiApplication::primaryScreen();
    const qreal dpr = target ? target->devicePixelRatio() : 1.0;

    const int px = static_cast<int>(std::lround(virtualRect.x() * dpr));
    const int py = static_cast<int>(std::lround(virtualRect.y() * dpr));
    const int w  = static_cast<int>(std::lround(virtualRect.width() * dpr));
    const int h  = static_cast<int>(std::lround(virtualRect.height() * dpr));
    if (w <= 0 || h <= 0) return {};

    ScreenDc screen;
    if (!screen.h) return {};

    MemDc mem(screen.h);
    if (!mem.h) return {};

    Bmp bmp{ ::CreateCompatibleBitmap(screen.h, w, h) };
    if (!bmp.h) return {};

    HGDIOBJ prev = ::SelectObject(mem.h, bmp.h);
    const BOOL ok = ::BitBlt(mem.h, 0, 0, w, h,
                             screen.h, px, py,
                             SRCCOPY | CAPTUREBLT);
    ::SelectObject(mem.h, prev);
    if (!ok) return {};

    QImage img(w, h, QImage::Format_ARGB32);
    if (img.isNull()) return {};

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;  // negative because GDI stores bottom-up by default. Welcome to 1992.
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    const int lines = ::GetDIBits(mem.h, bmp.h, 0, static_cast<UINT>(h),
                                  img.bits(), &bi, DIB_RGB_COLORS);
    if (lines != h) return {};

    // BitBlt leaves the alpha byte full of whatever was in memory. Slam it
    // to opaque or the resulting PNG will be a ghost.
    for (int y = 0; y < h; ++y) {
        auto* line = reinterpret_cast<quint32*>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            line[x] |= 0xFF000000u;
        }
    }
    return img;
}

QByteArray Capturer::toPngBase64(const QImage& img) {
    if (img.isNull()) return {};
    QByteArray png;
    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    if (!img.save(&buf, "PNG")) return {};
    return png.toBase64();
}
