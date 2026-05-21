#pragma once

#include <QObject>
#include <QString>

class AccessController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString role READ role WRITE setRole NOTIFY roleChanged)
    Q_PROPERTY(bool canRemoteOpen READ canRemoteOpen NOTIFY roleChanged)
    Q_PROPERTY(bool canExport READ canExport NOTIFY roleChanged)
    Q_PROPERTY(bool canChangeRole READ canChangeRole NOTIFY roleChanged)

public:
    explicit AccessController(QObject *parent = nullptr);

    QString role() const;
    void setRole(const QString &role);

    bool canRemoteOpen() const;
    bool canExport() const;
    bool canChangeRole() const;

    Q_INVOKABLE void cycleDemoRole();

signals:
    void roleChanged();

private:
    enum class Role { Owner, Family, Viewer };

    Role m_role = Role::Owner;

    static Role fromString(const QString &value);
    static QString toString(Role role);
};
