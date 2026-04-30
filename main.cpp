// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "DigitalClock.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    // Linux(Wayland/X11)環境でのみ、座標取得のためにxcbを強制
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif

    QApplication app(argc, argv);
    DigitalClock clock;
    clock.show();
    return app.exec();
}
