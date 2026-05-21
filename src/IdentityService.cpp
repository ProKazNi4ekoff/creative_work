#include "IdentityService.h"

#include <QRandomGenerator>

IdentityService::IdentityService(QObject *parent)
    : QObject(parent)
{
    m_ownerIds = {QStringLiteral("cat_murzik"),
                  QStringLiteral("cat_barsik"),
                  QStringLiteral("dog_rex")};
}

IdentityResult IdentityService::verify(const QString &petId) const
{
    IdentityResult result;
    result.isOwnerPet = m_ownerIds.contains(petId);

    if (result.isOwnerPet) {
        result.confidence = 0.85 + QRandomGenerator::global()->generateDouble() * 0.14;
    } else {
        result.confidence = 0.20 + QRandomGenerator::global()->generateDouble() * 0.45;
    }

    return result;
}
