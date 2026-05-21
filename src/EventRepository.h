#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QString>

class EventRepository : public QObject {
    Q_OBJECT

public:
    explicit EventRepository(QObject *parent = nullptr);

    bool init();
    bool addEvent(const QString &type,
                  const QString &message,
                  const QString &petId,
                  double confidence,
                  bool simulation);
    QVariantList loadEvents(int limit, bool simulation) const;
    QVariantList dailyStatsLastDays(int days = 14) const;
    bool exportCsv(const QString &filePath, bool mainOnly = true) const;
    bool clear(bool simulation);
    bool clearAll();

private:
    void ensureSimulationColumn();

    QSqlDatabase m_db;
};
