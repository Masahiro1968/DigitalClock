// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2026 Masahiro1968.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#pragma once

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
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
    enum SecondsMode { None, HalfSize, FullSize };

    DigitalClock(QWidget *parent = nullptr);
    void startApplication();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    QTime displayTime() const;
    void reverseColor(int pattern = -1);
    void loadPreference();
    void savePreference();

    void drawDigital(QPainter &painter);
    void drawColon(QPainter &painter, const QColor &color);
    void drawSegment(QPainter &painter, int digit, const QColor &color);

private:
    void updateWindowSize();

    int m_colorPattern;
    QColor m_hourColor;
    QColor m_minuteColor;
    QColor m_secondsColor;

    SecondsMode m_secondsMode;
    double m_baseScale;

    double m_sw;
    double m_st;
    double m_g;
    double m_digitMargin;

    // 【完全確定】1文字分の送り幅
    double getStepX() const { return 70.0 + m_digitMargin; }

    // 【完全確定】時分(4文字)が純粋に占める右端までの幅
    // (3文字分の送り ＋ コロンの送り ＋ 最後の4文字目自体の幅)
    double getBaseW() const
    {
        double singleDigitW = 50.0 + m_st;
        double colonW = 30.0 + m_digitMargin * 0.5;
        return (getStepX() * 3) + colonW + singleDigitW;
    }

    // 【完全確定】セグメントの縦幅
    double getBaseH() const { return 50.0 * 2 + m_st + 2.0 * 4; }

    QTime m_currentTime;
    QTimer *m_timer;
};
