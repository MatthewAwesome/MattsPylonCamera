#ifndef QPYLONCAMERA_H
#define QPYLONCAMERA_H

// We want to establish what this class will do. It will open the camera.
// And perhaps do other things, but this is our primary one.

#include <QObject>
#include <QAbstractVideoSurface>
#include <QAbstractVideoFilter>
#include <QVideoFilterRunnable>
#include <QVideoFrame>
#include <QImage>
#include <qqml.h>
#include <pylon/PylonIncludes.h>
#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>

using namespace Pylon;
using namespace GenApi;


/****************************BEGIN QPylonImgEventHandler CLASS DEFINITION *******************************************/


// Make a class to handle events: Not there is not constructor here.
// This class is a 'function' box.  No variables to initialize, no nothing to wire up.
class QPylonImgEventHandler : public QObject, public CImageEventHandler {

    Q_OBJECT
    friend class QPylonCamera;
    friend class VideoStream;

// Defining class roles:
public:
    using QObject::QObject;
    /**
     * @brief OnImageGrabbed
     * @param camera
     * @param ptrGrabResult
     *
     * This function communicates with the outside world.
     */
    virtual void OnImageGrabbed(CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult) override;

// Some privaate methods. We will leave this be for now.
private:
    static QImage resultToImage(const CGrabResultPtr& ptrGrabResult);
    static QImage toQImage(const CPylonImage &pylonImage);
signals:
    void frameGrabbed(const QImage &frame);
};


/****************************BEGIN VideoStream CLASS DEFINITION *******************************************/


// Defining a class to manage video stream.
class VideoStream : public QObject {

    Q_OBJECT
    Q_PROPERTY(QAbstractVideoSurface *videoSurface READ videoSurface \
               WRITE setVideoSurface NOTIFY videoSurfaceChanged)
    friend class QPylonCamera;

public:
    // Constructor-Destructor:
    explicit VideoStream(QObject *parent = Q_NULLPTR);
    virtual ~VideoStream();

    // Read video surface; or a pointer thereto:
    QAbstractVideoSurface *videoSurface() const;

    // Called upon QML creation of VideoOutput component.
    void setVideoSurface(QAbstractVideoSurface *videoSurface);

// Signals to do things...
signals:
    void startGrabbing();
    void videoSurfaceChanged();

// In general, limit public members.
public slots:

// Since we've declared PylonCamera a friend class, these can reside as private
private slots:
    Q_INVOKABLE bool startStream(const QRect &rect);
    Q_INVOKABLE void renderFrame(const QImage &frame);

private:
    QAbstractVideoSurface*  mVideoSurface;
};


/****************************BEGIN VideoFilter CLASS DEFINITION *******************************************/


class VideoFilter : public QAbstractVideoFilter
{
    Q_OBJECT
    friend class QPylonCamera;
public:
    VideoFilter( QObject* parent = nullptr );
    QVideoFilterRunnable* createFilterRunnable() Q_DECL_OVERRIDE;
};


/****************************BEGIN VideoFilterRunnable CLASS DEFINITION *******************************************/


class VideoFilterRunnable : public QVideoFilterRunnable
{
    friend class QPylonCamera;
public:
    VideoFilterRunnable();
    QVideoFrame run( QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags ) Q_DECL_OVERRIDE;
};


/****************************BEGIN CannyFilter CLASS DEFINITION *******************************************/


class CannyFilter : public QAbstractVideoFilter
{
    Q_OBJECT
    friend class QPylonCamera;
public:
    CannyFilter( QObject* parent = nullptr );
    QVideoFilterRunnable* createFilterRunnable() Q_DECL_OVERRIDE;
};


/****************************BEGIN VideoFilterRunnable CLASS DEFINITION *******************************************/


class CannyFilterRunnable : public QVideoFilterRunnable
{
    friend class QPylonCamera;
public:
    CannyFilterRunnable();
    QVideoFrame run( QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags ) Q_DECL_OVERRIDE;
private:
    int lowThreshold = 8;
    const int max_lowThreshold = 100;
    const int ratio = 3;
    const int kernel_size = 3;
};

/****************************BEGIN BWFilter CLASS DEFINITION *******************************************/


class BWFilter : public QAbstractVideoFilter
{
    Q_OBJECT
    friend class QPylonCamera;
public:
    BWFilter( QObject* parent = nullptr );
    QVideoFilterRunnable* createFilterRunnable() Q_DECL_OVERRIDE;
};


/****************************BEGIN BWFilterRunnable CLASS DEFINITION *******************************************/


class BWFilterRunnable : public QVideoFilterRunnable
{
    friend class QPylonCamera;
public:
    BWFilterRunnable();
    QVideoFrame run( QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags ) Q_DECL_OVERRIDE;
};

/****************************BEGIN HoughFilter CLASS DEFINITION *******************************************/


class HoughFilter : public QAbstractVideoFilter
{
    Q_OBJECT
    friend class QPylonCamera;
public:
    HoughFilter( QObject* parent = nullptr );
    QVideoFilterRunnable* createFilterRunnable() Q_DECL_OVERRIDE;
};


/****************************BEGIN HoughFilterRunnable CLASS DEFINITION *******************************************/


class HoughFilterRunnable : public QVideoFilterRunnable
{
    friend class QPylonCamera;
public:
    HoughFilterRunnable();
    QVideoFrame run( QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags ) Q_DECL_OVERRIDE;
};

/****************************BEGIN QPylonCamera CLASS DEFINITION *******************************************/


class QPylonCamera : public QObject {

    Q_OBJECT
    QML_ELEMENT

public:

    // Constructor-Destructor:
    explicit QPylonCamera(QObject *parent = Q_NULLPTR);
    virtual ~QPylonCamera();

    // Some state variables, and methods to return them:
    bool grabbing = false;
    Q_INVOKABLE inline bool isOpen() const {
        qDebug()<<"Is open?!"<<Qt::endl;
        return mCamera != Q_NULLPTR && mCamera->IsOpen();
    }
    Q_INVOKABLE inline bool isGrabbing() const {
        qDebug()<<"Is Streaming?!"<<Qt::endl;
        return grabbing;
    }

    // Methods that permit user interaction are below:

    // Establish communication with camera. Called via "Open" button.
    Q_INVOKABLE void open();

    // Terminate communication with camera.  Called via "Close" button.
    Q_INVOKABLE void close();

    // Iteratively grab frames and render them. Called via "Start Stream" button.
    Q_INVOKABLE void startStream();

    // Terminates grab session and destroys videoSurface. Called via "Stop Stream" button.
    Q_INVOKABLE void stopGrabbing();

    // QML, sometimes, needs to be provided objects. This function accomplishes this task.
    Q_INVOKABLE QObject* streamReturn(int s);


//    QVector<float> ellipseVector;
//    Q_INVOKABLE QVector<float> getEllipse();

// Wiring to other objects:
signals:
    // Provides mRect to the VideoStream class.
    void streamSignal(const QRect &rect);

// Reserved for future use:
public slots:

private slots:
    // When a signal is received here. the camera begins a grab session.
    // Or, more explicitely:
    /**
     * @brief startGrabbing
     * Start collectings frames from camera
     * And establish the connection between ImageHandler & VideoSurface
     * @return
     */
    bool startGrabbing();

// Members of our PylonCamera class, private ones:
private:
    QPylonImgEventHandler *mImageEventHandler;
    CInstantCamera*         mCamera;
    INodeMap              *nodemap;
    QRect                   mRect;
    VideoStream           *mVideoStreamOne;
    VideoStream           *mVideoStreamTwo;
    VideoStream           *mVideoStreamThree;
    VideoStream           *mVideoStreamFour;
};

#endif // QPYLONCAMERA_H
