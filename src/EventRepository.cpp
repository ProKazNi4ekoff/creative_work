#include "EventRepository.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTextStream>
#include <QVariantMap>

namespace {
QString escapeCsv(QString value)
{
    value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QLatin1Char('"') + value + QLatin1Char('"');
}
} // namespace

EventRepository::EventRepository(QObject *parent)
    : QObject(parent)
{
}

bool EventRepository::init()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base);

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    m_db.setDatabaseName(base + QStringLiteral("/cat_door_v2.db"));

    if (!m_db.open()) {
        qWarning() << "DB open error:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec(
            QStringLiteral("CREATE TABLE IF NOT EXISTS events ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "ts TEXT NOT NULL,"
                           "type TEXT NOT NULL,"
                           "message TEXT NOT NULL,"
                           "pet_id TEXT,"
                           "confidence REAL,"
                           "is_simulation INTEGER NOT NULL DEFAULT 0"
                           ")"))) {
        return false;
    }

    ensureSimulationColumn();
    return true;
}

void EventRepository::ensureSimulationColumn()
{
    QSqlQuery query(m_db);
    query.exec(QStringLiteral("PRAGMA table_info(events)"));
    bool hasColumn = false;
    while (query.next()) {
        if (query.value(1).toString() == QStringLiteral("is_simulation")) {
            hasColumn = true;
            break;
        }
    }
    if (!hasColumn) {
        QSqlQuery alter(m_db);
        alter.exec(QStringLiteral("ALTER TABLE events ADD COLUMN is_simulation INTEGER NOT NULL DEFAULT 0"));
    }
}

bool EventRepository::addEvent(const QString &type,
                               const QString &message,
                               const QString &petId,
                               double confidence,
                               bool simulation)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO events(ts,type,message,pet_id,confidence,is_simulation) VALUES(?,?,?,?,?,?)"));
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(type);
    query.addBindValue(message);
    query.addBindValue(petId);
    query.addBindValue(confidence);
    query.addBindValue(simulation ? 1 : 0);
    return query.exec();
}

QVariantList EventRepository::loadEvents(int limit, bool simulation) const
{
    QVariantList events;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT ts,type,message,pet_id,confidence FROM events "
        "WHERE is_simulation = ? ORDER BY id DESC LIMIT ?"));
    query.addBindValue(simulation ? 1 : 0);
    query.addBindValue(limit);

    if (!query.exec()) {
        return events;
    }

    while (query.next()) {
        QVariantMap row;
        const QDateTime timestamp =
            QDateTime::fromString(query.value(0).toString(), Qt::ISODate);
        row[QStringLiteral("ts")] =
            timestamp.isValid() ? timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                                : query.value(0).toString();
        row[QStringLiteral("type")] = query.value(1).toString();
        row[QStringLiteral("message")] = query.value(2).toString();
        row[QStringLiteral("petId")] = query.value(3).toString();
        row[QStringLiteral("confidence")] = query.value(4).toDouble();
        events.push_back(row);
    }

    return events;
}

QVariantList EventRepository::dailyStatsLastDays(int days) const
{
    QVariantList stats;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT date(ts) AS d,"
        "SUM(CASE WHEN type='pet_arrived_home' THEN 1 ELSE 0 END) AS arrivals,"
        "SUM(CASE WHEN type='pet_left_home' THEN 1 ELSE 0 END) AS exits,"
        "SUM(CASE WHEN type='door_opened' THEN 1 ELSE 0 END) AS opens "
        "FROM events "
        "WHERE is_simulation = 0 AND date(ts) >= date('now', ?) "
        "GROUP BY date(ts) "
        "ORDER BY d ASC"));
    query.addBindValue(QStringLiteral("-%1 day").arg(days));

    if (!query.exec()) {
        qWarning() << "dailyStats error:" << query.lastError().text();
        return stats;
    }

    while (query.next()) {
        QVariantMap row;
        row[QStringLiteral("day")] = query.value(0).toString();
        row[QStringLiteral("arrivals")] = query.value(1).toInt();
        row[QStringLiteral("exits")] = query.value(2).toInt();
        row[QStringLiteral("opens")] = query.value(3).toInt();
        stats.push_back(row);
    }

    return stats;
}

bool EventRepository::exportCsv(const QString &filePath, bool mainOnly) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << "ts,type,message,pet_id,confidence,is_simulation\n";

    QSqlQuery query(m_db);
    QString sql = QStringLiteral(
        "SELECT ts,type,message,pet_id,confidence,is_simulation FROM events ");
    if (mainOnly) {
        sql += QStringLiteral("WHERE is_simulation = 0 ");
    }
    sql += QStringLiteral("ORDER BY id ASC");

    if (!query.exec(sql)) {
        return false;
    }

    while (query.next()) {
        stream << escapeCsv(query.value(0).toString()) << ','
               << escapeCsv(query.value(1).toString()) << ','
               << escapeCsv(query.value(2).toString()) << ','
               << escapeCsv(query.value(3).toString()) << ','
               << QString::number(query.value(4).toDouble(), 'f', 3) << ','
               << query.value(5).toInt() << '\n';
    }

    return true;
}

bool EventRepository::clear(bool simulation)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM events WHERE is_simulation = ?"));
    query.addBindValue(simulation ? 1 : 0);
    return query.exec();
}

bool EventRepository::clearAll()
{
    QSqlQuery query(m_db);
    return query.exec(QStringLiteral("DELETE FROM events"));
}
