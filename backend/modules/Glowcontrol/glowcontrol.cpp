#include <QtDebug>
#include <unistd.h>
#include <algorithm>

#include "glowcontrol.h"

class Lightbulb;

GlowControl::GlowControl(QObject *parent) :
        QObject(parent),
        m_bulbs(this)
{
    qRegisterMetaType<lifx::Header>("lifx::Header");

    BulbTracker *tracker = new BulbTracker();
    tracker->moveToThread(&trackerThread);
    connect(&trackerThread, &QThread::finished, tracker, &QObject::deleteLater);
    connect(this, &GlowControl::requestTrackerStart, tracker, &BulbTracker::start);
    connect(this, &GlowControl::requestTrackerStop, tracker, &BulbTracker::stop);
    connect(tracker, &BulbTracker::bulbReady, this, &GlowControl::handleBulb);
    trackerThread.start();
    requestTrackerStart();

    // Worker is our worker, it does talking to bulbs etc.
    BulbWorker *worker = new BulbWorker();
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &GlowControl::dispatchJob, worker, &BulbWorker::doJob);
    workerThread.start();
}

BulbModel* GlowControl::bulbs() {
    return &m_bulbs;
}

void GlowControl::loseApplicationFocus() {
    requestTrackerStop();
}

void GlowControl::gainApplicationFocus() {
    requestTrackerStart();
}

void GlowControl::setListeners(Lightbulb * bulb) {
    connect(bulb, &Lightbulb::requestSetProperty, this, &GlowControl::bulbRequestsSetProperty);
}

void GlowControl::bulbRequestsSetProperty(const QString &key, const QVariant &value) {
    Lightbulb* bulb = qobject_cast<Lightbulb *>(QObject::sender());
    if (key == QStringLiteral("Label")) {
        // TODO: implement
    } else if (key == QStringLiteral("Power")) {
        emit dispatchJob(QStringLiteral("power"), value.toBool(), bulb->header);
    } else if (key == QStringLiteral("Color")) {
        emit dispatchJob(QStringLiteral("color"), value, bulb->header);
    } else if (key == QStringLiteral("Brightness")) {
        // We want to keep color information for brightness
        QVariantMap color = bulb->color().toMap();
        color["brightness"] = value.toInt();
        emit dispatchJob(QStringLiteral("brightness"), color, bulb->header);
    }
}

bool sort(Lightbulb* a, Lightbulb* b) {
    return a->label() < b->label();
}

void GlowControl::handleBulb(const QString &label, const bool power, const QVariant &color, const lifx::Header &header) {
    // m_bulblist << QString::fromStdString(bulb.label);

    // XXX: needs QPointer to guard
    // against dangling pointers if
    // a bulb qobj is ever deleted
    Lightbulb* bulb;
    if (!m_name_to_bulb.contains(label)) {
        bulb = new Lightbulb(this, header);
        setListeners(bulb);
        m_name_to_bulb.insert(label, bulb);
        bulb->lifxSetsProperty("Label", QVariant(label));
        m_bulbs.addBulb(bulb);
        qDebug() << "GlowControl, saw new bulb:" << label;
    } else {
        bulb = m_name_to_bulb[label];
    }

    bulb->lifxSetsProperty("Label", QVariant(label));
    bulb->lifxSetsProperty("Power", QVariant(power));
    QMap<QString, QVariant> colorMap = color.toMap();
    bulb->lifxSetsProperty("Color", color);
    bulb->lifxSetsProperty("Brightness", colorMap["brightness"]);
}

GlowControl::~GlowControl() {
    emit requestTrackerStop();
    trackerThread.quit();
    trackerThread.wait();
    workerThread.quit();
    workerThread.wait();
}
