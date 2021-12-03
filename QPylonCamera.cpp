#include "QPylonCamera.h"
#include <QDebug>
#include <QVideoSurfaceFormat>
#include <QPainter>
#include <QGuiApplication>
#include <QDateTime>
#include <QPixmap>
#include <QPainter>
#include <QStandardPaths>
//#include <opencv4/opencv2/highgui.hpp>
//#include <opencv4/opencv2/imgproc.hpp>

// Namespace for using GenApi objects. Helps us talk to the camera!
using namespace GenApi;

/****************************BEGIN QPylonImgEvent CLASS METHODS**************************************************/

// Callback to handle image-received responses. This function calls resultToImage,
// and resultToImage calls toQImage.
void QPylonImgEventHandler::OnImageGrabbed(CInstantCamera &camera, const CGrabResultPtr &ptrGrabResult)
{
    // frameGrabbed is called when camera is opened.
    Q_UNUSED(camera)
    // We call resultToImage and feed this result into frameGrabbed, which is a signal, that feeds to render frame.
    emit frameGrabbed(resultToImage(ptrGrabResult));
}

// Grabs camera data, turns it into an image. In this function, we save the image, which we can probably add with a
QImage QPylonImgEventHandler::resultToImage(const CGrabResultPtr &ptrGrabResult)
{
    if(!ptrGrabResult.IsValid()) {
        qWarning()<<"Invalid frame resultToImage";
        return QImage();
    }

    CImageFormatConverter fc;
    fc.OutputPixelFormat = PixelType_RGB8packed;

    CPylonImage pylonImage;

    fc.Convert(pylonImage, ptrGrabResult);
    if (!pylonImage.IsValid()) {
        qWarning()<<"Can't convert grabbed result to PylonImage";
        return QImage();
    }
    // We want to save the image here?
    QDateTime a = QDateTime::currentDateTime();
    std::string ac = a.toString("yyyyMMddHHmmmsszzz").append(".tiff").toStdString();
//    CImagePersistence::Save(ImageFileFormat_Tiff,String_t(ac.c_str()), pylonImage);
    return toQImage(pylonImage);
}

// Takes camera's image and makes a QImage out of it.
QImage QPylonImgEventHandler::toQImage(const CPylonImage &pylonImage)
{
    int width = pylonImage.GetWidth();
    int height = pylonImage.GetHeight();
    const void *buffer = pylonImage.GetBuffer();
    int step = pylonImage.GetAllocatedBufferSize() / height;
    return QImage(static_cast<const uchar*>(buffer),
                  width, height, step,
                  QImage::Format_RGB888).copy().convertToFormat(QImage::Format_RGB32);
}


/****************************BEGIN VideoStream CLASS METHODS**************************************************/

// VideoStream constructor:
VideoStream::VideoStream(QObject *parent):
    QObject(parent),
    mVideoSurface(Q_NULLPTR)
{
     qDebug()<<"VideoStream Constructor"<<Qt::endl;
}

VideoStream::~VideoStream(){}

// A function to update the videoSurface.
void VideoStream::setVideoSurface(QAbstractVideoSurface *videoSurface)
{
    qDebug()<<"Setting Video Surface"<<Qt::endl;
    if(videoSurface == mVideoSurface)
        return;

    mVideoSurface = videoSurface;

    if(mVideoSurface != Q_NULLPTR){
        emit videoSurfaceChanged();
    }
}

// Here we start the video surface:
bool VideoStream::startStream(const QRect &rect)
{
    qDebug()<<"VideoStream Starting"<<Qt::endl;
    if(mVideoSurface != Q_NULLPTR) {
        QVideoFrame::PixelFormat f = QVideoFrame::pixelFormatFromImageFormat(QImage::Format_RGB32);
        QVideoSurfaceFormat format(rect.size(), f);
        mVideoSurface->start(format);
        // Send signal to camerts to start grabbing. Or just call the function?
        emit startGrabbing();
        return true;
    } else {
        qWarning()<<"VideoSurface is null";
        return false;
    }
    // We next need to start send an event to the camera: Emit a signal to start grabbing:
}

void VideoStream::renderFrame(const QImage &frame)
{
    // Quick check for a live videosurface:
    if (!mVideoSurface)
        return;

    // Can we modify the frame there?

    // mVideoSurface->present(QVideoFrame(frame)) renders the frame.
    // And if this cannot be accomplished, we throw the warning:


    // Only action statement in this function, HERE!

    // Note, this function when excuted before a grab loop has been initialized,
    // will actually call start(), should a first frame be successfully rendered.
    if(!mVideoSurface->present(QVideoFrame(frame)))
        qWarning()<<"Can't render frame";
}

// A VideoSurface to render image data, frame by frame:
QAbstractVideoSurface *VideoStream::videoSurface() const
{
    return mVideoSurface;
}

/****************************BEGIN QPylonCamera CLASS METHODS**************************************************/

QPylonCamera::QPylonCamera(QObject *parent):
    QObject(parent),
    mImageEventHandler(new QPylonImgEventHandler(this)),
    mCamera(Q_NULLPTR),
    nodemap(),
    mRect(),
    mVideoStreamOne(new VideoStream(this)),
    mVideoStreamTwo(new VideoStream(this)),
    mVideoStreamThree(new VideoStream(this)),
    mVideoStreamFour(new VideoStream(this))
{
    PylonInitialize();
    qDebug()<<"QPylonCamera Constructor"<<Qt::endl;
    connect(this,&QPylonCamera::streamSignal,mVideoStreamOne,&VideoStream::startStream);
    connect(mVideoStreamOne,&VideoStream::startGrabbing, this,&QPylonCamera::startGrabbing);
    connect(this,&QPylonCamera::streamSignal,mVideoStreamTwo,&VideoStream::startStream);
    connect(this,&QPylonCamera::streamSignal,mVideoStreamThree,&VideoStream::startStream);
    connect(this,&QPylonCamera::streamSignal,mVideoStreamFour,&VideoStream::startStream);
}

QPylonCamera::~QPylonCamera(){}

// Opening and closing of camera connection:
void QPylonCamera::open()
{
     qDebug()<<"QPylonCamera Opening..."<<Qt::endl;
     if(isOpen())
         return;

     try {
         // Create an instant camera object with the camera device found first.
         mCamera = new CInstantCamera(CTlFactory::GetInstance().CreateFirstDevice());

         // Print the camera information.
         qInfo()<<"Using device : "<< mCamera->GetDeviceInfo().GetModelName();
         qInfo()<<"Friendly Name: "<< mCamera->GetDeviceInfo().GetFriendlyName();
         qInfo()<<"Full Name    : "<< mCamera->GetDeviceInfo().GetFullName();
         qInfo()<<"SerialNumber : "<< mCamera->GetDeviceInfo().GetSerialNumber();
//         qInfo()<<"Node Check   : "<< nodemap->GetDeviceName();

         // Handle image event.  Connection is established here via registration of mImageEvent handler.
         // As such 'control' of this function is handed off to the camera (Basler control).
         mCamera->RegisterImageEventHandler(mImageEventHandler, RegistrationMode_Append, Cleanup_Delete);

         // Begin communication:
         mCamera->Open();

         // nodemap of allows us to change camera parameters:
         nodemap = &mCamera->GetNodeMap();

         // Set the AOI:
         CIntegerParameter(nodemap, "Width").SetValue(1920);
         CIntegerParameter(nodemap, "Height").SetValue(1080);
         CIntegerParameter(nodemap, "OffsetX").SetValue(960);
         CIntegerParameter(nodemap, "OffsetY").SetValue(540);

         // Grab the values:
         CIntegerPtr offsetX( nodemap->GetNode( "OffsetX"));
         CIntegerPtr offsetY( nodemap->GetNode( "OffsetY"));
         CIntegerPtr width(   nodemap->GetNode( "Width"));
         CIntegerPtr height(  nodemap->GetNode( "Height"));
         CFloatPtr   gain(nodemap->GetNode("Gain"));

         // Print values out for sanity check:
         qInfo()<<"OffsetX: "<<offsetX->GetValue();
         qInfo()<<"OffsetY: "<<offsetY->GetValue();
         qInfo()<<"Width: "<<width->GetValue();
         qInfo()<<"Height: "<<height->GetValue();

         // A rectangle for our video stuffs to work with:
         mRect = QRect(offsetX->GetValue(),
                       offsetY->GetValue(),
                       width->GetValue(),
                       height->GetValue());

     }  catch (GenICam::GenericException &e) {
         mCamera = Q_NULLPTR;
         qWarning() << "Camera Error: " << e.GetDescription();
     }
}

void QPylonCamera::close()
{
    if(mCamera != Q_NULLPTR)
        qDebug()<<"QPylonCamera Closing..."<<Qt::endl;
    stopGrabbing();
    mCamera->Close();
    if(mVideoStreamOne->mVideoSurface != Q_NULLPTR)
        mVideoStreamOne->mVideoSurface->stop();
    if(mVideoStreamTwo->mVideoSurface != Q_NULLPTR)
        mVideoStreamTwo->mVideoSurface->stop();
    if(mVideoStreamThree->mVideoSurface != Q_NULLPTR)
        mVideoStreamThree->mVideoSurface->stop();
    if(mVideoStreamFour->mVideoSurface != Q_NULLPTR)
        mVideoStreamFour->mVideoSurface->stop();
}

// Called on button press in QML.
void QPylonCamera::startStream()
{
    qDebug()<<"Stream Starting"<<Qt::endl;
    emit streamSignal(mRect);

}

// Returns QAbstractVideoSurface to a QML VideoOutput component.
QObject* QPylonCamera::streamReturn(int s)
{
    switch(s) {
        case 0: return mVideoStreamOne;
        case 1: return mVideoStreamTwo;
        case 2: return mVideoStreamThree;
        case 3: return mVideoStreamFour;
        default: return mVideoStreamOne;
    }
}


// Tells the camera to begin a grab session.
bool QPylonCamera::startGrabbing()
{
    // If our camera isn't open, we return 0:
    if (!isOpen()) {
        qWarning()<<"Camera didn't open, can't start grabbing";
        return false;
    }

    // We our camera's buffer is filled with an image, we serve up said image and render it.
    connect(mImageEventHandler, &QPylonImgEventHandler::frameGrabbed,
            mVideoStreamOne, &VideoStream::renderFrame);
    connect(mImageEventHandler, &QPylonImgEventHandler::frameGrabbed,
            mVideoStreamTwo, &VideoStream::renderFrame);
    connect(mImageEventHandler, &QPylonImgEventHandler::frameGrabbed,
            mVideoStreamThree, &VideoStream::renderFrame);
    connect(mImageEventHandler, &QPylonImgEventHandler::frameGrabbed,
            mVideoStreamFour, &VideoStream::renderFrame);

    // Here we start a grabbing loop, and iteratively emit an mImageEventHandler signal,
    // which we have connected to the frameGrabbed signal, which eventually passes a frame
    // to the renderFrame function!
    mCamera->StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
    grabbing = true;
    return true;
};

// We call this function via QML. "Stop Stream" button.
void QPylonCamera::stopGrabbing(){
    // Of course, we can only do this if we have an open camera.
    if (!isOpen())
        return;

    // Disconnect the video surface and event handler:
    disconnect(mImageEventHandler, &QPylonImgEventHandler::frameGrabbed,
               mVideoStreamOne, &VideoStream::renderFrame);
    disconnect(mImageEventHandler, &QPylonImgEventHandler::frameGrabbed,
               mVideoStreamTwo, &VideoStream::renderFrame);
    disconnect(mImageEventHandler, &QPylonImgEventHandler::frameGrabbed,
               mVideoStreamThree, &VideoStream::renderFrame);
    disconnect(mImageEventHandler, &QPylonImgEventHandler::frameGrabbed,
               mVideoStreamFour, &VideoStream::renderFrame);

    // Updating states accordingly:

    // first the camera's state:
    if (mCamera->IsGrabbing())
        mCamera->StopGrabbing();

    // and then the videoSurface's state:
    if(mVideoStreamOne->mVideoSurface != Q_NULLPTR)
        mVideoStreamOne->mVideoSurface->stop();
    if(mVideoStreamTwo->mVideoSurface != Q_NULLPTR)
        mVideoStreamTwo->mVideoSurface->stop();
    if(mVideoStreamThree->mVideoSurface != Q_NULLPTR)
        mVideoStreamThree->mVideoSurface->stop();
    if(mVideoStreamFour->mVideoSurface != Q_NULLPTR)
        mVideoStreamFour->mVideoSurface->stop();

    // And lastly the state of our QPylonCamera class, the app's root contex!
    grabbing = false;
};


// VideoFilter Constructor:
VideoFilter::VideoFilter(QObject *parent): QAbstractVideoFilter( parent ){

}

// Method to make the filter runable:
QVideoFilterRunnable *VideoFilter::createFilterRunnable()
{
   return new VideoFilterRunnable();
}

// VideoFilterRunnable Constructor:
VideoFilterRunnable::VideoFilterRunnable(){

}

// Method to implement the filter: The magic happens here!
QVideoFrame VideoFilterRunnable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags)
{
    Q_UNUSED( surfaceFormat )
    Q_UNUSED( flags )

    // Making sure we've a videoframe to work with:
    if ( !input )
    {
        return QVideoFrame();
    }

    // Converting frame to image:
    QImage image = input->image();

    // Making sure we actually have an image, and returning something accordingly!
    if ( image.isNull() )
    {
        qDebug()<<"Null image in filter"<<Qt::endl;
        return QImage();
    }
    else{
        // Image dims:
        int w = image.width();
        int h = image.height();

        // Initiate painter:
        QPainter painter(&image);

        painter.setFont( QFont("Arial", 32 ) );
        painter.setPen( Qt::white );

        int cdim = 48;
        painter.fillRect( QRect( 0, 0, cdim, cdim ), QColor( Qt::GlobalColor::red ) );
        painter.drawText( QRect( 0, 0, cdim, cdim ), Qt::AlignCenter, QStringLiteral( "1" ) );

        painter.fillRect( QRect( w-1 - cdim, 0, cdim, cdim ), QColor( Qt::GlobalColor::darkGreen ) );
        painter.drawText( QRect( w-1 - cdim, 0, cdim, cdim ), Qt::AlignCenter, QStringLiteral( "2" ) );

        painter.fillRect( QRect( w-1 - cdim, h-1 - cdim, cdim, cdim ), QColor( Qt::GlobalColor::darkGreen ) );
        painter.drawText( QRect( w-1 - cdim, h-1 - cdim, cdim, cdim ), Qt::AlignCenter, QStringLiteral( "3" ) );

        painter.fillRect( QRect( 0, h-1 - cdim, cdim, cdim ), QColor( Qt::GlobalColor::blue ) );
        painter.drawText( QRect( 0, h-1 - cdim, cdim, cdim ), Qt::AlignCenter, QStringLiteral( "4" ) );
        return image;
    }
}

// Fleshing out the Canny edge detector:
CannyFilter::CannyFilter(QObject *parent): QAbstractVideoFilter( parent ){

}

QVideoFilterRunnable *CannyFilter::createFilterRunnable()
{
    return new CannyFilterRunnable();
}

CannyFilterRunnable::CannyFilterRunnable(){

}

QVideoFrame CannyFilterRunnable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags)
{
    Q_UNUSED( surfaceFormat )
    Q_UNUSED( flags )

    // Making sure we've a videoframe to work with:
    if ( !input )
    {
        return QVideoFrame();
    }

    // Converting frame to image:
    QImage image = input->image();

    // Making sure we actually have an image, and returning something accordingly!
    if ( image.isNull() )
    {
        qDebug()<<"Null image in filter"<<Qt::endl;
        return QImage();
    }
    else{
        // Image dims:
        int w = image.width();
        int h = image.height();

        // Converting QImage to cv::mat
        cv::Mat src, src_gray;
        cv::Mat dst,detected_edges;
        // For RGB32
        cv::Mat tmp(h,w,CV_8UC4,(uchar*)image.bits(),image.bytesPerLine());
        cv::cvtColor(tmp, src,cv::COLOR_RGBA2RGB);
        // For RGB:
//        cv::Mat tmp(image.height(),image.width(),CV_8UC3,(uchar*)image.bits(),image.bytesPerLine());

        // Using src as a template, constuct a destination matrix:
        dst.create( src.size(), src.type() );

        // Make the src image grayscale:
        cv::cvtColor( src, src_gray, cv::COLOR_RGB2GRAY );

        // Blue the grayed image:
        cv::blur( src_gray, detected_edges, cv::Size(3,3) );

        // Run Canny algorithm upon blurred image:
        cv::Canny( detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size );

        dst = cv::Scalar::all(0);

        src.copyTo( dst, detected_edges);

        // Converting cv::mat to QImage.
        cv::Mat temp,temp_rgba,output; // make the same cv::Mat
//        cv::cvtColor(src, temp,cv::COLOR_BGR2RGB);
        cv::cvtColor(detected_edges, temp_rgba,cv::COLOR_GRAY2RGBA);
        QImage dest((const uchar *) temp_rgba.data, temp_rgba.cols, temp_rgba.rows, temp_rgba.step, QImage::Format_RGB32);
        dest.bits(); // enforce deep copy, see documentation
        return dest;
    }
}

BWFilter::BWFilter(QObject *parent) : QAbstractVideoFilter(parent){

}

QVideoFilterRunnable *BWFilter::createFilterRunnable()
{
    return new BWFilterRunnable();
}

BWFilterRunnable::BWFilterRunnable(){

}

QVideoFrame BWFilterRunnable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags)
{
    Q_UNUSED( surfaceFormat )
    Q_UNUSED( flags )

    // Making sure we've a videoframe to work with:
    if ( !input )
    {
        return QVideoFrame();
    }

    // Converting frame to image:
    QImage image = input->image();

    // Making sure we actually have an image, and returning something accordingly!
    if ( image.isNull() )
    {
        qDebug()<<"Null image in filter"<<Qt::endl;
        return QImage();
    }
    else{
        // Convert to grayscale:
        QImage tmp    = image.convertToFormat(QImage::Format_Grayscale8);
        // Convert back to RGB32, since that is what is required by the video surface.
        QImage output = tmp.convertToFormat(QImage::Format_RGB32);
        // Render a BW image.
        return output;
    }
}

HoughFilter::HoughFilter(QObject *parent) : QAbstractVideoFilter(parent){

}

QVideoFilterRunnable *HoughFilter::createFilterRunnable()
{
    return new HoughFilterRunnable();
}


HoughFilterRunnable::HoughFilterRunnable(){

}

QVideoFrame HoughFilterRunnable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags)
{
    Q_UNUSED( surfaceFormat )
    Q_UNUSED( flags )

    // Making sure we've a videoframe to work with:
    if ( !input )
    {
        return QVideoFrame();
    }

    // Converting frame to image:
    QImage image = input->image();

    // Making sure we actually have an image, and returning something accordingly!
    if ( image.isNull() )
    {
        qDebug()<<"Null image in filter"<<Qt::endl;
        return QImage();
    }
    else{
        // Here we perform a hough transform:
        int w = image.width();
        int h = image.height();

        // Take QImage, return cv::Mat.
        cv::Mat tmp(h,w,CV_8UC4,(uchar*)image.bits(),image.bytesPerLine());

        // Make Grayscale:
        cv::Mat gray;
        cv::cvtColor(tmp,gray,cv::COLOR_RGBA2GRAY);

        // Blur grayscale image: (denoising, sort of)
        cv::medianBlur(gray, gray, 5);
        cv::Mat blur;
        cv::cvtColor(gray,blur,cv::COLOR_GRAY2RGBA);

        // Now do the transform:
        std::vector<cv::Vec3f> circles;
        HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1,
                     gray.rows/16,  // change this value to detect circles with different distances to each other
                     100, 30, 0, 0 // change the last two parameters
                // (min_radius & max_radius) to detect larger circles
        );

        // Draw circles on the image:
        for( size_t i = 0; i < circles.size(); i++ )
        {
            cv::Vec3i c = circles[i];
            cv::Point center = cv::Point(c[0], c[1]);
            // circle center
            circle( tmp, center, 1, cv::Scalar(0,100,100), 3, cv::LINE_AA);
            // circle outline
            int radius = c[2];
            circle( tmp, center, radius, cv::Scalar(255,0,255), 3, cv::LINE_AA);
        }

        // And output the image; circles drawn!
        QImage output((const uchar *) tmp.data, tmp.cols, tmp.rows, tmp.step, QImage::Format_RGB32);
        output.bits(); // enforce deep copy, see documentation
        return output;
    }
}
