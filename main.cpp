#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "QPylonCamera.h"
int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);
    qmlRegisterType<VideoFilter>("Filters", 1, 0, "VideoFilter" );
    qmlRegisterType<CannyFilter>("Filters", 1, 0, "CannyFilter" );
    qmlRegisterType<BWFilter>("Filters", 1, 0, "BWFilter" );
    qmlRegisterType<HoughFilter>("Filters", 1, 0, "HoughFilter" );
    QQmlApplicationEngine engine;

    // We can, by virtue of rootContext, call methods of pylonCamera in QML.
    engine.rootContext()->setContextProperty("pylonCamera", new QPylonCamera);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));


    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
