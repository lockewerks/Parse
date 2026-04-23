// Parse - Locke Werks
// Copyright (c) 2026

#pragma once
#include <QWidget>
#include <QRect>
#include <QPoint>

class RegionSelector : public QWidget {
    Q_OBJECT
public:
    explicit RegionSelector(QWidget* parent = nullptr);
    void start();

signals:
    void regionSelected(const QRect& virtualRect);
    void cancelled();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    QRect currentRect() const;
    QRect virtualScreenRect() const;

    QPoint m_origin;
    QPoint m_current;
    bool m_dragging = false;
};
