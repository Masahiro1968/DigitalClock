// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2026 Masahiro1968.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "DigitalClock.h"
#include <QCoreApplication>

DigitalClock::DigitalClock(QWidget *parent)
    : QWidget(parent)
    , m_colorPattern(0)
    , m_hourColor(palette().color(QPalette::Text))
    , m_minuteColor(palette().color(QPalette::Text))
    , m_secondsColor(palette().color(QPalette::Accent))
    , m_secondsMode(DigitalClock::HalfSize)
    , m_baseScale(1.15)
    , m_st(12.0)
    , m_digitMargin(0.0) // 初期値はマージンなし（これまでの標準幅）
    , m_timer(nullptr)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle(tr("Digital Clock"));
}

void DigitalClock::startApplication()
{
    loadPreference();
    updateWindowSize();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, QOverload<>::of(&DigitalClock::update));
    m_timer->start(250);

    update();
}

void DigitalClock::updateWindowSize()
{
    if (m_st <= 2)
        m_st = 2;
    if (m_digitMargin < 0)
        m_digitMargin = 0;
    if (m_digitMargin > 25.0)
        m_digitMargin = 25.0;

    // 1. 基本サイズ（時分がピタッと収まる幅）
    double logicalW = getBaseW();

    // 2. 秒のモードに応じて、増える領域を「最後の文字の右端」まで1ドット単位でシミュレート
    if (m_secondsMode == FullSize) {
        double colonW = 30.0 + m_digitMargin * 0.5;
        double singleDigitW = 50.0 + m_st;
        // 分の最後の送り幅 ＋ コロンの送り幅 ＋ 秒1文字目の送り幅 ＋ 秒2文字目の純粋な幅
        logicalW += (getStepX() - singleDigitW) + colonW + getStepX() + singleDigitW;
    } else if (m_secondsMode == HalfSize) {
        double colonW = (30.0 + m_digitMargin * 0.5) * 0.5;
        double singleDigitW = 50.0 + m_st;
        // 分の最後の送り幅 ＋ ハーフコロン送り ＋ 秒1文字目ハーフ送り ＋ 秒2文字目のハーフサイズ純粋幅
        logicalW += (getStepX() - singleDigitW) + colonW + (getStepX() * 0.5)
                    + (singleDigitW * 0.5);
    }

    // 左右にほんの少しだけブレインマージン（左右均等に固定で 10px ずつ程度）を持たせる
    logicalW += 20.0;

    int targetW = static_cast<int>(logicalW * m_baseScale);
    int targetH = static_cast<int>((getBaseH() + 8.0) * m_baseScale); // 上下余白分+8

    resize(qMax(50, targetW), qMax(30, targetH));
}

void DigitalClock::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), QColor(0, 0, 0, 1));
    m_currentTime = displayTime();

    double scale = m_baseScale;

    // ウィンドウサイズと完全に一致する論理幅を再計算
    double logicalW = getBaseW();
    if (m_secondsMode == FullSize) {
        double colonW = 30.0 + m_digitMargin * 0.5;
        double singleDigitW = 50.0 + m_st;
        logicalW += (getStepX() - singleDigitW) + colonW + getStepX() + singleDigitW;
    } else if (m_secondsMode == HalfSize) {
        double colonW = (30.0 + m_digitMargin * 0.5) * 0.5;
        double singleDigitW = 50.0 + m_st;
        logicalW += (getStepX() - singleDigitW) + colonW + (getStepX() * 0.5)
                    + (singleDigitW * 0.5);
    }
    logicalW += 20.0;

    QRect r = contentsRect();
    double tx = (r.width() - (logicalW * scale)) / 2.0;
    double ty = (r.height() - ((getBaseH() + 8.0) * scale)) / 2.0;

    // 左右均等に綺麗に収まるように中央へ translate (+10.0 で左余白を相殺)
    painter.translate(tx + 10.0 * scale, ty + 4.0 * scale);
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
    Qt::KeyboardModifiers modifiers = event->modifiers();
    double delta = event->angleDelta().y() > 0 ? 1.0 : -1.0;

    // 1. Ctrl + Shift + ホイール = 「太さ (st)」の変更
    if ((modifiers & Qt::ControlModifier) && (modifiers & Qt::ShiftModifier)) {
        m_st = qMax(2.0, m_st + delta);
    }
    // 2. Ctrl + ホイール = 「数字同士の間隔 (m_digitMargin)」の変更！
    else if (modifiers & Qt::ControlModifier) {
        m_digitMargin = qBound(0.0, m_digitMargin + delta * 2.0, 25.0); // 2px刻みで綺麗に開閉
    }
    // 3. 通常のホイール = 全体の拡大・縮小
    else {
        double scaleDelta = event->angleDelta().y() > 0 ? 0.05 : -0.05;
        m_baseScale = qMax(0.4, m_baseScale + scaleDelta);
    }

    updateWindowSize();
    update();
}

void DigitalClock::mouseDoubleClickEvent(QMouseEvent *) {}

void DigitalClock::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    QMenu *secondsMenu = menu.addMenu("Seconds Display");
    QAction *actNone = secondsMenu->addAction("Hide (なし)", this, [this]() {
        m_secondsMode = None;
        updateWindowSize();
    });
    QAction *actHalf = secondsMenu->addAction("Half Size (半分)", this, [this]() {
        m_secondsMode = HalfSize;
        updateWindowSize();
    });
    QAction *actFull = secondsMenu->addAction("Full Size (同じ)", this, [this]() {
        m_secondsMode = FullSize;
        updateWindowSize();
    });

    actNone->setCheckable(true);
    actNone->setChecked(m_secondsMode == None);
    actHalf->setCheckable(true);
    actHalf->setChecked(m_secondsMode == HalfSize);
    actFull->setCheckable(true);
    actFull->setChecked(m_secondsMode == FullSize);

    menu.addSeparator();
    menu.addAction("Reset Ratio (黄金比リセット)", this, [this]() {
        m_st = 12.0;
        m_digitMargin = 0.0;
        updateWindowSize();
        update();
    });

    menu.addSeparator();
    menu.addAction("Light/Dark", this, [this]() { reverseColor(); });
    menu.addSeparator();
    menu.addAction("Quit", this, [this]() {
        if (m_timer) {
            m_timer->stop();
        }
        close();
    });
    menu.exec(event->globalPos());
}

void DigitalClock::closeEvent(QCloseEvent *event)
{
    savePreference();
    event->accept();
}

QTime DigitalClock::displayTime() const
{
    return QTime::currentTime();
}

void DigitalClock::reverseColor(int pattern)
{
    if (pattern == -1) {
        m_colorPattern = (m_colorPattern == 0) ? 1 : 0;
    } else {
        m_colorPattern = pattern;
    }

    if (m_colorPattern == 1) {
        m_hourColor = palette().color(QPalette::Light);
        m_minuteColor = palette().color(QPalette::Light);
        m_secondsColor = palette().color(QPalette::Accent);
    } else {
        m_hourColor = palette().color(QPalette::Text);
        m_minuteColor = palette().color(QPalette::Text);
        m_secondsColor = palette().color(QPalette::Accent);
    }
    update();
}

void DigitalClock::loadPreference()
{
#ifdef Q_OS_LINUX
    QSettings settings(OWNER_NAME, APP_NAME);
#else
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, OWNER_NAME, APP_NAME);
#endif

    if (settings.contains("windowPos")) {
        move(settings.value("windowPos").toPoint());
    }
    m_baseScale = settings.value("baseScale", 1.15).toDouble();
    m_secondsMode = static_cast<SecondsMode>(settings.value("secondsMode", HalfSize).toInt());

    // ⭕️ この2つだけをスマートに復元
    m_st = settings.value("segmentST", 12.0).toDouble();
    m_digitMargin = settings.value("digitMargin", 0.0).toDouble();

    int savedPattern = settings.value("colorPattern", 0).toInt();
    reverseColor(savedPattern);
}

void DigitalClock::savePreference()
{
#ifdef Q_OS_LINUX
    QSettings settings(OWNER_NAME, APP_NAME);
#else
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, OWNER_NAME, APP_NAME);
#endif

    settings.setValue("windowPos", pos());
    settings.setValue("baseScale", m_baseScale);
    settings.setValue("secondsMode", static_cast<int>(m_secondsMode));
    settings.setValue("colorPattern", m_colorPattern);

    // ⭕️ この2つだけをスマートに保存
    settings.setValue("segmentST", m_st);
    settings.setValue("digitMargin", m_digitMargin);

    settings.sync();
}

void DigitalClock::drawDigital(QPainter &painter)
{
    painter.save();

    QString s = m_currentTime.toString("HHmmss");
    if (s.length() < 6)
        return;

    bool showColon = m_currentTime.second() % 2 == 0;
    const double stepX = 70.0 + m_digitMargin;
    int loopCount = (m_secondsMode == None) ? 4 : 6;
    const double digitH = getBaseH();

    for (int i = 0; i < loopCount; ++i) {
        int val = s[i].digitValue();
        QColor c = (i < 2) ? m_hourColor : (i < 4) ? m_minuteColor : m_secondsColor;

        painter.save();

        // 秒（5, 6文字目）かつハーフサイズモードなら下揃え縮小
        if (i >= 4 && m_secondsMode == HalfSize) {
            painter.translate(0, digitH * 0.5);
            painter.scale(0.5, 0.5);
        }

        // 影
        painter.save();
        painter.translate(2, 2);
        drawSegment(painter, 8, QColor(0, 0, 0, 40));
        painter.restore();

        // 本体
        drawSegment(painter, val, c);
        painter.restore();

        // 次の桁への移動
        if (i >= 4 && m_secondsMode == HalfSize) {
            painter.translate(stepX * 0.5, 0);
        } else {
            painter.translate(stepX, 0);
        }

        // コロンの挿入チェック
        // 【修正】i == 3 (分と秒の間) のコロンは、秒なし(None)モードではない時だけ描画・移動する！
        if (i == 1 || (i == 3 && m_secondsMode != None)) {
            painter.save();

            if (i == 3 && m_secondsMode == HalfSize) {
                painter.translate(0, digitH * 0.5);
                painter.scale(0.5, 0.5);
            }

            if (showColon) {
                drawColon(painter, c);
            }
            painter.restore();

            // コロンの幅だけ座標を進める
            if (i == 3 && m_secondsMode == HalfSize) {
                painter.translate((30.0 + m_digitMargin * 0.5) * 0.5, 0);
            } else {
                painter.translate(30.0 + m_digitMargin * 0.5, 0);
            }
        }
    }
    painter.restore();
}

void DigitalClock::drawColon(QPainter &painter, const QColor &color)
{
    painter.save();
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);

    const int size = static_cast<int>(m_st);
    const int x = 10;
    const int y_upper = static_cast<int>(50.0 * 0.8); // sw=50固定
    const int y_lower = static_cast<int>(50.0 * 1.8);

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

    static const unsigned char patterns[10]
        = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

    // 数字単体の美しさを100%保つため、ここは黄金比で完全固定
    const int sw = 50;
    const int st = static_cast<int>(m_st); // 太さだけが気持ちよく連動
    const int g = 2;

    auto draw = [&](int segIndex, const QPolygon &poly, int tx, int ty) {
        painter.save();
        painter.translate(tx, ty);
        bool isOn = (patterns[value % 10] >> segIndex) & 0x01;
        painter.setBrush(isOn ? color : QColor(80, 80, 80, 20));
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(poly);
        painter.restore();
    };

    QPolygon h, v;
    h << QPoint(st / 2, 0) << QPoint(sw - st / 2, 0) << QPoint(sw, st / 2)
      << QPoint(sw - st / 2, st) << QPoint(st / 2, st) << QPoint(0, st / 2);
    v << QPoint(st / 2, 0) << QPoint(st, st / 2) << QPoint(st, sw - st / 2) << QPoint(st / 2, sw)
      << QPoint(0, sw - st / 2) << QPoint(0, st / 2);

    draw(0, h, st / 2 + g, 0);
    draw(1, v, sw + g, st / 2 + g);
    draw(2, v, sw + g, sw + st / 2 + g * 3);
    draw(3, h, st / 2 + g, sw * 2 + g * 4);
    draw(4, v, 0, sw + st / 2 + g * 3);
    draw(5, v, 0, st / 2 + g);
    draw(6, h, st / 2 + g, sw + g * 2);

    painter.restore();
}
