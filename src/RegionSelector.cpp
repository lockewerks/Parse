// Parse - Locke Werks
// Copyright (c) 2026

#include "RegionSelector.h"

#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>

// The window-flag cocktail below is load-bearing: translucent, always on
// top, invisible to the window manager, and styled as a tool so it doesn't
// show in the taskbar. Drop any one of these and you'll discover a
// delightful new failure mode.
RegionSelector::RegionSelector(QWidget* parent)
    : QWidget(parent,
              Qt::FramelessWindowHint
              | Qt::WindowStaysOnTopHint
              | Qt::Tool
              | Qt::BypassWindowManagerHint) {
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

QRect RegionSelector::virtualScreenRect() const {
    QRect u;
    for (const QScreen* s : QGuiApplication::screens()) {
        u = u.united(s->geometry());
    }
    return u;
}

void RegionSelector::start() {
    m_dragging = false;
    m_origin = m_current = QPoint();

    const QRect geom = virtualScreenRect();
    setGeometry(geom);
    showFullScreen();
    raise();
    activateWindow();
    grabKeyboard();
    update();
}

QRect RegionSelector::currentRect() const {
    return QRect(m_origin, m_current).normalized();
}

void RegionSelector::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), QColor(0, 0, 0, 110));

    if (!m_dragging) return;

    const QRect sel = currentRect();

    // Punch a transparent hole where the user dragged. CompositionMode_Clear
    // is the only incantation that actually clears filled pixels on a
    // translucent widget. Plain `fillRect(..., Qt::transparent)` does nothing.
    p.setCompositionMode(QPainter::CompositionMode_Clear);
    p.fillRect(sel, Qt::transparent);

    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    QPen pen(QColor(255, 255, 255, 220));
    pen.setWidth(1);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(sel.adjusted(0, 0, -1, -1));

    const QString label = QString::number(sel.width()) + QChar('x')
                          + QString::number(sel.height());
    QFontMetrics fm(font());
    const QRect textRect(m_current.x() + 12, m_current.y() + 12,
                         fm.horizontalAdvance(label) + 10, fm.height() + 4);
    p.fillRect(textRect, QColor(0, 0, 0, 160));
    p.setPen(QColor(230, 230, 230));
    p.drawText(textRect, Qt::AlignCenter, label);
}

void RegionSelector::mousePressEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) return;
    m_origin = e->pos();
    m_current = e->pos();
    m_dragging = true;
    update();
}

void RegionSelector::mouseMoveEvent(QMouseEvent* e) {
    if (!m_dragging) return;
    m_current = e->pos();
    update();
}

void RegionSelector::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton || !m_dragging) return;
    m_dragging = false;

    const QRect local = currentRect();
    releaseKeyboard();
    hide();

    if (local.width() >= 5 && local.height() >= 5) {
        const QRect virt = local.translated(geometry().topLeft());
        emit regionSelected(virt);
    } else {
        emit cancelled();
    }
}

void RegionSelector::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) {
        m_dragging = false;
        releaseKeyboard();
        hide();
        emit cancelled();
    }
}
