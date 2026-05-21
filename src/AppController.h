#pragma once

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

#include "AccessController.h"
#include "DoorController.h"
#include "EventRepository.h"
#include "IdentityService.h"

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString doorState READ doorState NOTIFY doorStateChanged)
    Q_PROPERTY(bool obstacle READ obstacle WRITE setObstacle NOTIFY obstacleChanged)
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)
    Q_PROPERTY(QVariantList simEvents READ simEvents NOTIFY simEventsChanged)
    Q_PROPERTY(QVariantList notifications READ notifications NOTIFY notificationsChanged)
    Q_PROPERTY(QVariantList simNotifications READ simNotifications NOTIFY simNotificationsChanged)
    Q_PROPERTY(QVariantList dailyStats READ dailyStats NOTIFY dailyStatsChanged)
    Q_PROPERTY(QString role READ role WRITE setRole NOTIFY accessChanged)
    Q_PROPERTY(bool canRemoteOpen READ canRemoteOpen NOTIFY accessChanged)
    Q_PROPERTY(bool canExport READ canExport NOTIFY accessChanged)
    Q_PROPERTY(QString lastHomeEntry READ lastHomeEntry NOTIFY statsChanged)
    Q_PROPERTY(QString lastExit READ lastExit NOTIFY statsChanged)
    Q_PROPERTY(QString timeOutside READ timeOutside NOTIFY statsChanged)
    Q_PROPERTY(bool simulationRunning READ simulationRunning NOTIFY simulationRunningChanged)
    Q_PROPERTY(bool closeRetryPending READ closeRetryPending NOTIFY closeRetryPendingChanged)
    Q_PROPERTY(bool waitingForOpenDoor READ waitingForOpenDoor NOTIFY waitingForOpenDoorChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    QString doorState() const;
    bool obstacle() const;
    QVariantList events() const;
    QVariantList simEvents() const;
    QVariantList notifications() const;
    QVariantList simNotifications() const;
    QVariantList dailyStats() const;
    QString role() const;
    void setRole(const QString &role);
    bool canRemoteOpen() const;
    bool canExport() const;
    QString lastHomeEntry() const;
    QString lastExit() const;
    QString timeOutside() const;
    bool simulationRunning() const;
    bool closeRetryPending() const;
    bool waitingForOpenDoor() const;

    Q_INVOKABLE void openDoor();
    Q_INVOKABLE void closeDoor();
    Q_INVOKABLE void simulateChipAtDoor();
    Q_INVOKABLE void setObstacle(bool value);
    Q_INVOKABLE void refreshDailyStats();
    Q_INVOKABLE bool exportCsv(const QUrl &fileUrl);
    Q_INVOKABLE void cycleDemoRole();
    Q_INVOKABLE void clearMainHistory();
    Q_INVOKABLE void clearSimHistory();
    Q_INVOKABLE void startAutoSimulation();
    Q_INVOKABLE void stopAutoSimulation();

signals:
    void doorStateChanged();
    void obstacleChanged();
    void eventsChanged();
    void simEventsChanged();
    void notificationsChanged();
    void simNotificationsChanged();
    void dailyStatsChanged();
    void accessChanged();
    void statsChanged();
    void simulationRunningChanged();
    void closeRetryPendingChanged();
    void waitingForOpenDoorChanged();

private slots:
    void onDoorEvent(const QString &type, const QString &message);
    void onCloseFailed();
    void onCloseRetryTimer();
    void onAutoSimulationTick();

private:
    void appendMainEvent(const QString &type,
                         const QString &message,
                         const QString &petId = QString(),
                         double confidence = 0.0);
    void appendSimEvent(const QString &type,
                        const QString &message,
                        const QString &petId = QString(),
                        double confidence = 0.0);
    void notifyMain(const QString &text, const QString &category = QStringLiteral("info"));
    void notifySim(const QString &text);
    void recordEntryAtDoorOpen(const QString &timestamp);
    void recordExitAtDoorOpen(const QString &timestamp);
    void processChipRead(const QString &petId);
    void pauseAutoSimForUserAction();
    void resumeAutoSimIfNeeded();
    void scheduleCloseRetry();
    void recomputeStats();

    DoorController m_door;
    IdentityService m_identity;
    EventRepository m_repo;
    AccessController m_access;

    QVariantList m_events;
    QVariantList m_simEvents;
    QVariantList m_notifications;
    QVariantList m_simNotifications;
    QVariantList m_dailyStats;

    QString m_lastHomeEntry = QStringLiteral("-");
    QString m_lastExit = QStringLiteral("-");
    QString m_timeOutside = QStringLiteral("-");
    QDateTime m_exitTimestamp;

    QTimer m_autoTimer;
    QTimer m_closeRetryTimer;
    bool m_closeRetryPending = false;
    bool m_autoSimEnabled = false;
    bool m_autoSimPaused = false;
    bool m_waitingOpenAfterChip = false;
    bool m_petIsHome = false;
    QString m_pendingPetId;
};
