#include "AppController.h"

#include <QVariantMap>

namespace {
constexpr int kCloseRetryMs = 15000;
constexpr int kAutoSimIntervalMs = 8000;
}

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    m_repo.init();
    m_events = m_repo.loadEvents(500, false);
    m_simEvents = m_repo.loadEvents(500, true);
    m_dailyStats = m_repo.dailyStatsLastDays();

    connect(&m_door, &DoorController::stateChanged, this, &AppController::doorStateChanged);
    connect(&m_door, &DoorController::obstacleDetectedChanged, this, &AppController::obstacleChanged);
    connect(&m_door, &DoorController::doorEvent, this, &AppController::onDoorEvent);
    connect(&m_door, &DoorController::closeFailed, this, &AppController::onCloseFailed);

    m_autoTimer.setInterval(kAutoSimIntervalMs);
    connect(&m_autoTimer, &QTimer::timeout, this, &AppController::onAutoSimulationTick);

    m_closeRetryTimer.setSingleShot(true);
    m_closeRetryTimer.setInterval(kCloseRetryMs);
    connect(&m_closeRetryTimer, &QTimer::timeout, this, &AppController::onCloseRetryTimer);

    recomputeStats();
}

QString AppController::doorState() const
{
    return m_door.state();
}

bool AppController::obstacle() const
{
    return m_door.obstacleDetected();
}

QVariantList AppController::events() const
{
    return m_events;
}

QVariantList AppController::simEvents() const
{
    return m_simEvents;
}

QVariantList AppController::notifications() const
{
    return m_notifications;
}

QVariantList AppController::simNotifications() const
{
    return m_simNotifications;
}

QVariantList AppController::dailyStats() const
{
    return m_dailyStats;
}

QString AppController::role() const
{
    return m_access.role();
}

void AppController::setRole(const QString &role)
{
    m_access.setRole(role);
    emit accessChanged();
}

bool AppController::canRemoteOpen() const
{
    return m_access.canRemoteOpen();
}

bool AppController::canExport() const
{
    return m_access.canExport();
}

QString AppController::lastHomeEntry() const
{
    return m_lastHomeEntry;
}

QString AppController::lastExit() const
{
    return m_lastExit;
}

QString AppController::timeOutside() const
{
    return m_timeOutside;
}

bool AppController::simulationRunning() const
{
    return m_autoSimEnabled;
}

bool AppController::closeRetryPending() const
{
    return m_closeRetryPending;
}

bool AppController::waitingForOpenDoor() const
{
    return m_waitingOpenAfterChip;
}

void AppController::appendMainEvent(const QString &type,
                                    const QString &message,
                                    const QString &petId,
                                    double confidence)
{
    QVariantMap row;
    const QString timestamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    row[QStringLiteral("ts")] = timestamp;
    row[QStringLiteral("type")] = type;
    row[QStringLiteral("message")] = message;
    row[QStringLiteral("petId")] = petId;
    row[QStringLiteral("confidence")] = confidence;

    m_events.prepend(row);
    if (m_events.size() > 400) {
        m_events.removeLast();
    }

    m_repo.addEvent(type, message, petId, confidence, false);
    emit eventsChanged();

    if (type == QStringLiteral("pet_arrived_home")) {
        m_lastHomeEntry = timestamp;
        if (m_exitTimestamp.isValid()) {
            const qint64 seconds = m_exitTimestamp.secsTo(QDateTime::currentDateTime());
            const int minutes = static_cast<int>(seconds / 60);
            m_timeOutside = QStringLiteral("%1 мин").arg(minutes);
            m_exitTimestamp = QDateTime();
        }
        emit statsChanged();
    } else if (type == QStringLiteral("pet_left_home")) {
        m_lastExit = timestamp;
        m_exitTimestamp = QDateTime::currentDateTime();
        m_timeOutside = QStringLiteral("сейчас на улице");
        emit statsChanged();
    }

    refreshDailyStats();
}

void AppController::appendSimEvent(const QString &type,
                                   const QString &message,
                                   const QString &petId,
                                   double confidence)
{
    QVariantMap row;
    row[QStringLiteral("ts")] =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    row[QStringLiteral("type")] = type;
    row[QStringLiteral("message")] = message;
    row[QStringLiteral("petId")] = petId;
    row[QStringLiteral("confidence")] = confidence;

    m_simEvents.prepend(row);
    if (m_simEvents.size() > 400) {
        m_simEvents.removeLast();
    }

    m_repo.addEvent(type, message, petId, confidence, true);
    emit simEventsChanged();
}

void AppController::notifyMain(const QString &text, const QString &category)
{
    QVariantMap row;
    row[QStringLiteral("ts")] =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    row[QStringLiteral("text")] = text;
    row[QStringLiteral("category")] = category;
    m_notifications.prepend(row);
    if (m_notifications.size() > 100) {
        m_notifications.removeLast();
    }
    emit notificationsChanged();
}

void AppController::notifySim(const QString &text)
{
    QVariantMap row;
    row[QStringLiteral("ts")] =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    row[QStringLiteral("text")] = text;
    m_simNotifications.prepend(row);
    if (m_simNotifications.size() > 100) {
        m_simNotifications.removeLast();
    }
    emit simNotificationsChanged();
}

void AppController::recordEntryAtDoorOpen(const QString &timestamp)
{
    appendMainEvent(QStringLiteral("pet_arrived_home"),
                    QStringLiteral("Вход домой (дверь открыта)"));
    notifyMain(QStringLiteral("Вход домой: %1").arg(timestamp), QStringLiteral("entry"));
    m_petIsHome = true;
}

void AppController::recordExitAtDoorOpen(const QString &timestamp)
{
    appendMainEvent(QStringLiteral("pet_left_home"),
                    QStringLiteral("Выход на улицу (дверь открыта)"));
    notifyMain(QStringLiteral("Выход на улицу: %1").arg(timestamp), QStringLiteral("exit"));
    m_petIsHome = false;
}

void AppController::onDoorEvent(const QString &type, const QString &message)
{
    appendMainEvent(type, message);

    if (type == QStringLiteral("door_locked")) {
        m_closeRetryTimer.stop();
        if (m_closeRetryPending) {
            m_closeRetryPending = false;
            emit closeRetryPendingChanged();
        }
        notifyMain(QStringLiteral("Дверь успешно закрыта."), QStringLiteral("door"));
    } else if (type == QStringLiteral("door_opened")) {
        notifyMain(QStringLiteral("Дверь открыта."), QStringLiteral("door"));
    }
}

void AppController::onCloseFailed()
{
    notifyMain(QStringLiteral("Дверь не закрылась — в проёме препятствие. Повтор через 15 сек."),
               QStringLiteral("alert"));
    appendMainEvent(QStringLiteral("door_close_failed"), QStringLiteral("Не удалось закрыть дверь"));

    if (obstacle()) {
        appendSimEvent(QStringLiteral("sim_obstacle_block"),
                       QStringLiteral("Закрытие заблокировано (препятствие в симуляции)"));
        notifySim(QStringLiteral("Симуляция: препятствие мешает закрытию, повтор через 15 сек."));
    }

    scheduleCloseRetry();
}

void AppController::scheduleCloseRetry()
{
    if (m_closeRetryPending) {
        return;
    }
    m_closeRetryPending = true;
    emit closeRetryPendingChanged();
    m_closeRetryTimer.start();
}

void AppController::onCloseRetryTimer()
{
    m_closeRetryPending = false;
    emit closeRetryPendingChanged();

    if (doorState() == QStringLiteral("Locked")) {
        return;
    }

    notifyMain(QStringLiteral("Повторная попытка закрыть дверь…"), QStringLiteral("door"));
    closeDoor();
}

void AppController::openDoor()
{
    if (!m_access.canRemoteOpen()) {
        notifyMain(QStringLiteral("Недостаточно прав для удалённого открытия."), QStringLiteral("alert"));
        appendMainEvent(QStringLiteral("access_denied"), QStringLiteral("Отказ в доступе"));
        return;
    }

    const QString ts =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    m_door.requestOpen(5000);
    appendMainEvent(QStringLiteral("door_open_command"), QStringLiteral("Команда «Открыть дверь»"));

    if (m_waitingOpenAfterChip) {
        recordEntryAtDoorOpen(ts);
        m_waitingOpenAfterChip = false;
        emit waitingForOpenDoorChanged();
        appendSimEvent(QStringLiteral("sim_user_opened"),
                       QStringLiteral("Хозяин открыл дверь после считывания чипа"),
                       m_pendingPetId);
        notifySim(QStringLiteral("Дверь открыта по вашей команде, вход зафиксирован."));
        m_pendingPetId.clear();
        resumeAutoSimIfNeeded();
    } else if (m_petIsHome) {
        recordExitAtDoorOpen(ts);
        appendSimEvent(QStringLiteral("sim_exit_via_door"),
                       QStringLiteral("Выход: дверь открыта, питомец дома был"));
    } else {
        recordEntryAtDoorOpen(ts);
    }
}

void AppController::closeDoor()
{
    if (!m_access.canRemoteOpen()) {
        notifyMain(QStringLiteral("Недостаточно прав для управления дверью."), QStringLiteral("alert"));
        return;
    }

    m_door.requestClose();
    appendMainEvent(QStringLiteral("door_close_command"), QStringLiteral("Команда «Закрыть дверь»"));
    notifyMain(QStringLiteral("Команда «Закрыть дверь» отправлена."), QStringLiteral("door"));
}

void AppController::simulateChipAtDoor()
{
    processChipRead(QStringLiteral("cat_murzik"));
}

void AppController::processChipRead(const QString &petId)
{
    const IdentityResult identity = m_identity.verify(petId);
    const QString chipMsg = QStringLiteral("Считывание RFID: %1 (уверенность %2)")
                              .arg(petId)
                              .arg(identity.confidence, 0, 'f', 2);
    appendSimEvent(QStringLiteral("sim_chip_read"), chipMsg, petId, identity.confidence);

    if (!identity.isOwnerPet) {
        appendSimEvent(QStringLiteral("sim_chip_rejected"),
                       QStringLiteral("Чип не в белом списке — доступ запрещён"),
                       petId,
                       identity.confidence);
        notifySim(QStringLiteral("Неизвестный чип. Дверь не открывается."));
        return;
    }

    appendSimEvent(QStringLiteral("sim_chip_accepted"),
                   QStringLiteral("Свой питомец подтверждён по чипу"),
                   petId,
                   identity.confidence);
    notifySim(QStringLiteral("Чип принят. Ожидание команды «Открыть дверь» на главной."));

    m_pendingPetId = petId;
    m_waitingOpenAfterChip = true;
    emit waitingForOpenDoorChanged();

    notifyMain(QStringLiteral("Ваш питомец у двери (чип RFID). Нажмите «Открыть»."),
               QStringLiteral("entry"));

    pauseAutoSimForUserAction();
}

void AppController::pauseAutoSimForUserAction()
{
    if (m_autoTimer.isActive()) {
        m_autoTimer.stop();
        m_autoSimPaused = true;
    }
}

void AppController::resumeAutoSimIfNeeded()
{
    if (m_autoSimEnabled && m_autoSimPaused && !m_waitingOpenAfterChip) {
        m_autoTimer.start();
        m_autoSimPaused = false;
        appendSimEvent(QStringLiteral("sim_auto_resumed"),
                       QStringLiteral("Автосимуляция продолжена после вашего действия"));
    }
}

void AppController::setObstacle(bool value)
{
    m_door.setObstacleDetected(value);
    appendSimEvent(QStringLiteral("sim_obstacle"),
                   value ? QStringLiteral("Препятствие в проёме: ВКЛ")
                         : QStringLiteral("Препятствие в проёме: ВЫКЛ"));
    notifySim(value ? QStringLiteral("Препятствие включено — закрытие будет блокироваться.")
                    : QStringLiteral("Препятствие выключено."));
}

void AppController::refreshDailyStats()
{
    m_dailyStats = m_repo.dailyStatsLastDays(14);
    emit dailyStatsChanged();
}

bool AppController::exportCsv(const QUrl &fileUrl)
{
    if (!m_access.canExport()) {
        notifyMain(QStringLiteral("Экспорт недоступен для вашей роли."), QStringLiteral("alert"));
        return false;
    }

    const QString path = fileUrl.toLocalFile();
    if (path.isEmpty()) {
        return false;
    }

    const bool ok = m_repo.exportCsv(path, true);
    notifyMain(ok ? QStringLiteral("История приложения экспортирована в CSV.")
                   : QStringLiteral("Не удалось экспортировать CSV."),
                QStringLiteral("info"));
    return ok;
}

void AppController::cycleDemoRole()
{
    m_access.cycleDemoRole();
    notifyMain(QStringLiteral("Роль: ") + m_access.role(), QStringLiteral("info"));
    emit accessChanged();
}

void AppController::clearMainHistory()
{
    m_repo.clear(false);
    m_events.clear();
    m_notifications.clear();
    m_lastHomeEntry = QStringLiteral("-");
    m_lastExit = QStringLiteral("-");
    m_timeOutside = QStringLiteral("-");
    m_exitTimestamp = QDateTime();
    m_petIsHome = false;
    m_waitingOpenAfterChip = false;
    m_pendingPetId.clear();
    emit waitingForOpenDoorChanged();

    emit eventsChanged();
    emit notificationsChanged();
    emit statsChanged();
    refreshDailyStats();
}

void AppController::clearSimHistory()
{
    m_repo.clear(true);
    m_simEvents.clear();
    m_simNotifications.clear();
    emit simEventsChanged();
    emit simNotificationsChanged();
}

void AppController::startAutoSimulation()
{
    if (!m_autoSimEnabled) {
        m_autoSimEnabled = true;
        m_autoSimPaused = false;
        m_autoTimer.start();
        appendSimEvent(QStringLiteral("sim_started"), QStringLiteral("Автосимуляция запущена"));
        notifySim(QStringLiteral("Автосимуляция: считывание чипа, затем ожидание «Открыть»."));
        emit simulationRunningChanged();
    }
}

void AppController::stopAutoSimulation()
{
    if (m_autoSimEnabled) {
        m_autoSimEnabled = false;
        m_autoSimPaused = false;
        m_autoTimer.stop();
        appendSimEvent(QStringLiteral("sim_stopped"), QStringLiteral("Автосимуляция остановлена"));
        notifySim(QStringLiteral("Автосимуляция остановлена."));
        emit simulationRunningChanged();
    }
}

void AppController::onAutoSimulationTick()
{
    if (m_waitingOpenAfterChip) {
        return;
    }

    simulateChipAtDoor();
}

void AppController::recomputeStats()
{
    for (const QVariant &item : m_events) {
        const QVariantMap row = item.toMap();
        if (m_lastHomeEntry == QStringLiteral("-")
            && row.value(QStringLiteral("type")).toString() == QStringLiteral("pet_arrived_home")) {
            m_lastHomeEntry = row.value(QStringLiteral("ts")).toString();
        }
        if (m_lastExit == QStringLiteral("-")
            && row.value(QStringLiteral("type")).toString() == QStringLiteral("pet_left_home")) {
            m_lastExit = row.value(QStringLiteral("ts")).toString();
        }
    }
}
