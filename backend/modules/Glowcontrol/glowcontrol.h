/*
The MIT License (MIT)
Copyright © 2015 Jonas G. Drange <jonas@drange.net>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

Glow control is the controller. It dispatches jobs (power up/down, brightness n, etc)
to the worker and starts/stops the tracker.
*/

#ifndef GLOWCONTROL_H
#define GLOWCONTROL_H

#include <QObject>
#include <QThread>
#include <QQmlListProperty>
#include <lib-lifx/lifx.h>

#include "bulbtracker.h"
#include "bulbworker.h"
#include "bulbmodel.h"
#include "lightbulb.h"

#define GLOWCONTROL_VERSION "0.3.1";

class GlowControl : public QObject
{
    Q_OBJECT
    Q_PROPERTY(BulbModel* bulbs READ bulbs CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    QThread trackerThread;
    QThread workerThread;

public:
    explicit GlowControl(QObject *parent = 0);
    ~GlowControl();
    BulbModel* bulbs();
    Q_INVOKABLE void loseApplicationFocus();
    Q_INVOKABLE void gainApplicationFocus();
    QString version() { return GLOWCONTROL_VERSION; }

Q_SIGNALS:
    void requestTrackerStart();
    void requestTrackerStop();
    void dispatchJob(const QString &type, const QVariant &arg, const lifx::Header &header);

public slots:
    void handleBulb(const QString &label, const bool power, const QVariant &color, const lifx::Header &header);
    void bulbRequestsSetProperty(const QString &key, const QVariant &value);

private:
    void setListeners(Lightbulb * bulb);
    QMap<QString, Lightbulb *> m_name_to_bulb;
    BulbModel m_bulbs;
};

#endif // GLOWCONTROL_H
