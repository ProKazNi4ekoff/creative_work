#pragma once

#include <QObject>
#include <QSet>
#include <QString>

struct IdentityResult {
    bool isOwnerPet = false;
    double confidence = 0.0;
};

class IdentityService : public QObject {
    Q_OBJECT

public:
    explicit IdentityService(QObject *parent = nullptr);

    IdentityResult verify(const QString &petId) const;

private:
    QSet<QString> m_ownerIds;
};
