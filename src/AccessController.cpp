#include "AccessController.h"

AccessController::AccessController(QObject *parent)
    : QObject(parent)
{
}

QString AccessController::role() const
{
    return toString(m_role);
}

void AccessController::setRole(const QString &role)
{
    const Role next = fromString(role);
    if (m_role == next) {
        return;
    }
    m_role = next;
    emit roleChanged();
}

bool AccessController::canRemoteOpen() const
{
    return m_role == Role::Owner || m_role == Role::Family;
}

bool AccessController::canExport() const
{
    return m_role == Role::Owner || m_role == Role::Family;
}

bool AccessController::canChangeRole() const
{
    return m_role == Role::Owner;
}

void AccessController::cycleDemoRole()
{
    if (!canChangeRole()) {
        return;
    }

    if (m_role == Role::Owner) {
        m_role = Role::Family;
    } else if (m_role == Role::Family) {
        m_role = Role::Viewer;
    } else {
        m_role = Role::Owner;
    }

    emit roleChanged();
}

AccessController::Role AccessController::fromString(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("family")) {
        return Role::Family;
    }
    if (normalized == QStringLiteral("viewer")) {
        return Role::Viewer;
    }
    return Role::Owner;
}

QString AccessController::toString(Role role)
{
    switch (role) {
    case Role::Owner:
        return QStringLiteral("Owner");
    case Role::Family:
        return QStringLiteral("Family");
    case Role::Viewer:
        return QStringLiteral("Viewer");
    }
    return QStringLiteral("Owner");
}
