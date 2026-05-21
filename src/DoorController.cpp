#include "DoorController.h"

DoorController::DoorController(QObject *parent)
    : QObject(parent)
{
    m_openHold.setSingleShot(true);
    m_closingAnim.setSingleShot(true);
    m_blockedHold.setSingleShot(true);

    connect(&m_openHold, &QTimer::timeout, this, &DoorController::onOpenHoldExpired);
    connect(&m_closingAnim, &QTimer::timeout, this, &DoorController::onClosingTimerExpired);
    connect(&m_blockedHold, &QTimer::timeout, this, &DoorController::onBlockedHoldExpired);
}

QString DoorController::state() const
{
    return toString(m_state);
}

bool DoorController::obstacleDetected() const
{
    return m_obstacleDetected;
}

QString DoorController::toString(State state) const
{
    switch (state) {
    case State::Locked:
        return QStringLiteral("Locked");
    case State::Open:
        return QStringLiteral("Open");
    case State::Closing:
        return QStringLiteral("Closing");
    case State::Blocked:
        return QStringLiteral("Blocked");
    }
    return QStringLiteral("Unknown");
}

void DoorController::setState(State state)
{
    if (m_state == state) {
        return;
    }

    m_state = state;
    emit stateChanged();

    switch (state) {
    case State::Locked:
        m_openHold.stop();
        m_closingAnim.stop();
        m_blockedHold.stop();
        emit doorEvent(QStringLiteral("door_locked"), QStringLiteral("Дверь закрыта и заблокирована"));
        break;
    case State::Open:
        emit doorEvent(QStringLiteral("door_opened"), QStringLiteral("Дверь открыта"));
        scheduleClose();
        break;
    case State::Closing:
        emit doorEvent(QStringLiteral("door_closing"), QStringLiteral("Дверь закрывается"));
        m_closingAnim.start(1200);
        break;
    case State::Blocked:
        m_closingAnim.stop();
        emit doorEvent(QStringLiteral("obstacle_blocked"),
                       QStringLiteral("Препятствие в проёме — закрытие отменено"));
        m_blockedHold.start(1000);
        break;
    }
}

void DoorController::scheduleClose()
{
    m_openHold.stop();
    m_openHold.start(m_openHoldMs);
}

void DoorController::requestOpen(int openMs)
{
    m_openHoldMs = openMs;

    if (m_state == State::Open) {
        scheduleClose();
        return;
    }

    if (m_state == State::Closing) {
        m_closingAnim.stop();
    }

    if (m_state == State::Blocked) {
        m_blockedHold.stop();
    }

    setState(State::Open);
}

void DoorController::requestClose()
{
    if (m_state == State::Locked) {
        return;
    }

    m_openHold.stop();
    m_blockedHold.stop();

    if (m_obstacleDetected) {
        setState(State::Blocked);
        emit closeFailed();
        return;
    }

    if (m_state == State::Open || m_state == State::Blocked) {
        setState(State::Closing);
        return;
    }

    if (m_state == State::Closing) {
        return;
    }
}

void DoorController::lock()
{
    if (m_state == State::Locked) {
        return;
    }
    setState(State::Locked);
}

void DoorController::setObstacleDetected(bool value)
{
    if (m_obstacleDetected == value) {
        return;
    }
    m_obstacleDetected = value;
    emit obstacleDetectedChanged();
    onObstacleChanged();
}

void DoorController::onOpenHoldExpired()
{
    if (m_state != State::Open) {
        return;
    }

    if (m_obstacleDetected) {
        setState(State::Blocked);
        emit closeFailed();
    } else {
        setState(State::Closing);
    }
}

void DoorController::onClosingTimerExpired()
{
    if (m_state != State::Closing) {
        return;
    }

    if (m_obstacleDetected) {
        setState(State::Blocked);
        emit closeFailed();
    } else {
        setState(State::Locked);
    }
}

void DoorController::onBlockedHoldExpired()
{
    if (m_state != State::Blocked) {
        return;
    }
    setState(State::Open);
}

void DoorController::onObstacleChanged()
{
    if (m_state == State::Closing && m_obstacleDetected) {
        m_closingAnim.stop();
        setState(State::Blocked);
        emit closeFailed();
    }
}
