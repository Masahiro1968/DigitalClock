// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2026 Masahiro1968.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "DigitalClock.h"

DigitalClock::DigitalClock(QWidget *parent)
    : QWidget(parent)
    , m_colorPattern(0)
    , m_hourColor(palette().color(QPalette::Text))
    , m_minuteColor(palette().color(QPalette::Text))
    , m_secondsColor(palette().color(QPalette::Accent))
{
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&DigitalClock::update));
    timer->start(40);

    setWindowTitle(tr("Digital Clock"));
    resize(m_startW, m_startH);
    loadPreference();

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
}

void DigitalClock::paintEvent(QPaintEvent *)
{
    // ... ストップウォッチ処理 ...
    m_currentTime = displayTime();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    double scaleX = double(width()) / m_startW;
    double scaleY = double(height()) / m_startH;
    double scale = qMin(scaleX, scaleY) * 0.94;
    painter.scale(scale, scale);
    drawDigital(painter);
}

void DigitalClock::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (auto handle = windowHandle()) {
            handle->startSystemMove();
            event->accept();
        }
    }
}

void DigitalClock::wheelEvent(QWheelEvent *event)
{
    // 1. 現在のアスペクト比を計算、または固定値を定義
    // デジタル時計なら「幅3 : 高さ1」くらいが収まりが良いです
    const double aspectRatio = 3.0 / 1.0;

    // 2. 変化量を計算
    int delta = event->angleDelta().y() > 0 ? 20 : -20;

    // 3. 新しい幅を決定（最小幅を制限）
    int newWidth = qMax(200, width() + delta);

    // 4. 幅に合わせて高さを算出
    int newHeight = static_cast<int>(newWidth / aspectRatio);

    // 5. リサイズ実行
    resize(newWidth, newHeight);
}

void DigitalClock::mouseDoubleClickEvent(QMouseEvent *event)
{
    // future implement.
}

void DigitalClock::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    if (m_isStopwatchMode) {
        if (m_isRunning) {
            menu.addAction("Stop", this, &DigitalClock::stopStopwatch);
        } else {
            menu.addAction("Start", this, &DigitalClock::startStopwatch);
        }
        menu.addAction("Reset", this, &DigitalClock::resetStopwatch);
        menu.addSeparator();
        menu.addAction("Return to Clock", this, &DigitalClock::switchToClockMode);
    } else {
        menu.addAction("Stopwatch Mode", this, &DigitalClock::startStopwatch);
    }

    menu.addSeparator();
    menu.addAction("Light/Dark", this, [this]() { reverseColor(); });
    menu.addSeparator();
    menu.addAction("Quit", qApp, &QCoreApplication::quit);
    menu.exec(event->globalPos());
}

void DigitalClock::closeEvent(QCloseEvent *event)
{
    savePreference();
    event->accept();
}

QTime DigitalClock::displayTime() const
{
    if (m_isStopwatchMode) {
        return m_stopwatchElapsed;
    }
    return QTime::currentTime();
}

void DigitalClock::startStopwatch()
{
    if (!m_isStopwatchMode)
        m_stopwatchElapsed = QTime(0, 0);

    m_isStopwatchMode = true;
    m_isRunning = true;
    m_elapsedTimer.start();

    update();
}

void DigitalClock::stopStopwatch()
{
    m_isRunning = false;
}

void DigitalClock::resetStopwatch()
{
    m_isRunning = false;
    m_stopwatchElapsed = QTime(0, 0);
    update();
}

void DigitalClock::switchToClockMode()
{
    m_isRunning = false;
    m_isStopwatchMode = false;
    update();
}

void DigitalClock::reverseColor(int pattern)
{
    if (pattern == -1) {
        m_colorPattern = (m_colorPattern == 0) ? 1 : 0;
    } else {
        m_colorPattern = pattern;
    }

    if (m_colorPattern == 1) {
        // Light pattern
        m_hourColor = palette().color(QPalette::Light);
        m_minuteColor = palette().color(QPalette::Light);
        m_secondsColor = palette().color(QPalette::Accent);
    } else {
        // Dark pattern
        m_hourColor = palette().color(QPalette::Text);
        m_minuteColor = palette().color(QPalette::Text);
        m_secondsColor = palette().color(QPalette::Accent);
    }
    update();
}

void DigitalClock::loadPreference()
{
    QSettings settings(OWNER_NAME, APP_NAME);

    if (settings.contains("windowPos")) {
        move(settings.value("windowPos").toPoint());
        resize(settings.value("windowSize").toSize());
    }

    int savedPattern = settings.value("colorPattern", 0).toInt();
    reverseColor(savedPattern);
}

void DigitalClock::savePreference()
{
    QSettings settings(OWNER_NAME, APP_NAME);

    settings.setValue("windowPos", pos());
    settings.setValue("windowSize", size());
    settings.setValue("colorPattern", m_colorPattern);
    settings.sync();
}

void DigitalClock::drawDigital(QPainter &painter)
{
    painter.save();

    // ウィンドウの左上からのマージンを直接指定（ここで隙間を追い込めます）
    const int marginLeft = 10;
    const int marginTop = 20;
    painter.translate(marginLeft, marginTop);

    QString s = m_currentTime.toString("HHmmss");

    for (int i = 0; i < 6; ++i) {
        int val = s[i].digitValue();
        QColor c = (i < 2) ? m_hourColor : (i < 4) ? m_minuteColor : m_secondsColor;

        painter.save();

        // --- ここでドロップシャドウ（影）の描画 ---
        // わずかに右下にずらして、非常に薄い色で背景セグメントを描く
        painter.save();
        painter.translate(2, 2);
        drawSegment(painter, 8, QColor(0, 0, 0, 40)); // 常に「8」を描いて全セグメントの影にする
        painter.restore();

        // 本体の描画
        drawSegment(painter, val, c);

        painter.restore();

        // 次の桁へ移動（送り幅）
        painter.translate(70, 0);
        if (i == 1 || i == 3) {
            // コロンの描画処理など
            painter.translate(30, 0);
        }
    }
    painter.restore();
}

void DigitalClock::drawColon(QPainter &painter, const QColor &color)
{
    painter.save();
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);

    // コロンの形状（小さな六角形を2つ）
    const int size = 12;
    const int x = 10;
    const int y_upper = 40;
    const int y_lower = 90;

    auto drawDot = [&](int ty) {
        QPolygon p;
        p << QPoint(size / 2, 0) << QPoint(size, size / 2) << QPoint(size / 2, size)
          << QPoint(0, size / 2);
        painter.save();
        painter.translate(x, ty);
        painter.drawPolygon(p);
        painter.restore();
    };

    drawDot(y_upper);
    drawDot(y_lower);

    painter.restore();
}

void DigitalClock::drawSegment(QPainter &painter, int value, const QColor &color)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    // 数値(0-9)ごとの点灯パターン [A, B, C, D, E, F, G]
    static const unsigned char patterns[10]
        = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

    const int sw = 50; // 長さ
    const int st = 12; // 太さ
    const int g = 2;   // 隙間

    auto draw = [&](int segIndex, const QPolygon &poly, int tx, int ty) {
        painter.save();
        painter.translate(tx, ty);
        bool isOn = (patterns[value % 10] >> segIndex) & 0x01;

        // 消灯時は背景に馴染む程度の暗い色（透過あり）にするとリアルです
        painter.setBrush(isOn ? color : QColor(80, 80, 80, 20));
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(poly);
        painter.restore();
    };

    // 形状定義
    QPolygon h, v;
    h << QPoint(st / 2, 0) << QPoint(sw - st / 2, 0) << QPoint(sw, st / 2)
      << QPoint(sw - st / 2, st) << QPoint(st / 2, st) << QPoint(0, st / 2);
    v << QPoint(st / 2, 0) << QPoint(st, st / 2) << QPoint(st, sw - st / 2) << QPoint(st / 2, sw)
      << QPoint(0, sw - st / 2) << QPoint(0, st / 2);

    // 各セグメントを絶対位置（左上0,0基準）で配置
    draw(0, h, st / 2 + g, 0);               // A (Top)
    draw(1, v, sw + g, st / 2 + g);          // B (Right Top)
    draw(2, v, sw + g, sw + st / 2 + g * 3); // C (Right Bottom)
    draw(3, h, st / 2 + g, sw * 2 + g * 4);  // D (Bottom)
    draw(4, v, 0, sw + st / 2 + g * 3);      // E (Left Bottom)
    draw(5, v, 0, st / 2 + g);               // F (Left Top)
    draw(6, h, st / 2 + g, sw + g * 2);      // G (Middle)

    painter.restore();
}
