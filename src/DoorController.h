#pragma once

#include <QObject>
#include <QTimer>

class DoorController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool obstacleDetected READ obstacleDetected WRITE setObstacleDetected NOTIFY obstacleDetectedChanged)

public:
    explicit DoorController(QObject *parent = nullptr);

    QString state() const;
    bool obstacleDetected() const;

    Q_INVOKABLE void requestOpen(int openMs = 5000);
    Q_INVOKABLE void requestClose();
    Q_INVOKABLE void lock();
    Q_INVOKABLE void setObstacleDetected(bool value);

signals:
    void stateChanged();
    void obstacleDetectedChanged();
    void doorEvent(const QString &type, const QString &message);
    void closeFailed();

private slots:
    void onOpenHoldExpired();
    void onClosingTimerExpired();
    void onBlockedHoldExpired();
    void onObstacleChanged();

private:
    enum class State { Locked, Open, Closing, Blocked };

    void setState(State state);
    void scheduleClose();
    QString toString(State state) const;

    State m_state = State::Locked;
    bool m_obstacleDetected = false;
    int m_openHoldMs = 5000;

    QTimer m_openHold;
    QTimer m_closingAnim;
    QTimer m_blockedHold;
};
