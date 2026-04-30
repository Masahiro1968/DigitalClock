// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2026 Masahiro1968.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#pragma once

#include <QElapsedTimer>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QWindow>

#define OWNER_NAME "Masahiro1968"
#define APP_NAME "DigitalClock"

class DigitalClock : public QWidget
{
    Q_OBJECT

public:
    DigitalClock(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    QTime displayTime() const;
    void startStopwatch();
    void stopStopwatch();
    void resetStopwatch();
    void switchToClockMode();
    void reverseColor(int pattern = -1);
    void loadPreference();
    void savePreference();

    void drawDigital(QPainter &painter);
    void drawColon(QPainter &painter, const QColor &color);
    void drawSegment(QPainter &painter, int digit, const QColor &color);

private:

    QColor m_hourColor;
    QColor m_minuteColor;
    QColor m_secondsColor;
    int m_colorPattern;

    const double m_startW = 470.0;
    const double m_startH = 100.0;

    QTime m_currentTime;
    QTime m_stopwatchElapsed;
    QElapsedTimer m_elapsedTimer;
    bool m_isStopwatchMode = false;
    bool m_isRunning = false;
};
