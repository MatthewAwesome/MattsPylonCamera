#ifndef QIMAGEPAINTER_H
#define QIMAGEPAINTER_H

#include <QObject>
#include <QVideoFrame>
#include <QVideoSurfaceFormat>
#include <QPainter>

class QImagePainter : public QPainter
{
public:
    explicit QImagePainter( QImage* image = nullptr, QVideoFrame* videoFrame = nullptr, const QVideoSurfaceFormat& surfaceFormat = QVideoSurfaceFormat(), int videoOutputOrientation = 0 );

signals:

};
#endif
