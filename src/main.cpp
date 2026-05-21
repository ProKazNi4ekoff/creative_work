#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "AppController.h"

static QUrl resolveMainQmlUrl()
{
    if (QFile::exists(QStringLiteral(":/qml/Main.qml"))) {
        return QUrl(QStringLiteral("qrc:/qml/Main.qml"));
    }
    const QString diskPath = QDir::cleanPath(
        QCoreApplication::applicationDirPath() + QStringLiteral("/../../qml/Main.qml"));
    if (QFileInfo::exists(diskPath)) {
        return QUrl::fromLocalFile(diskPath);
    }
    return {};
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("CatDoor"));
    QCoreApplication::setApplicationName(QStringLiteral("CatDoorVirtualV2"));

    AppController controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            qCritical() << "QQmlApplicationEngine: не удалось создать корневой компонент";
        },
        Qt::QueuedConnection);

    const QUrl mainUrl = resolveMainQmlUrl();
    if (!mainUrl.isValid()) {
        qCritical() << "Main.qml не найден (ни qrc, ни ../../qml/Main.qml)";
        return -1;
    }

    engine.load(mainUrl);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "QML не загрузился. URL:" << mainUrl;
        return -1;
    }

    return app.exec();
}
