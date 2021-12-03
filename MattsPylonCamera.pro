# We are going to take action here.

QT += quick multimedia gui

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Defining Pylon stuff here:
!defined(PYLON_DEV_DIR) {
    PYLON_DEV_DIR = $$shell_path(/opt/pylon)
}

# Make sure we can include pylon headers, etc.
INCLUDEPATH +=  $$shell_path($$PYLON_DEV_DIR/include)
INCLUDEPATH +=  $$shell_path(/usr/local/include/opencv4)

# Need to be more verbose about pylon libs with unix.
unix{
    PYLON_LIBS = $$shell_path($$PYLON_DEV_DIR/lib)
    message("unix build")
    LIBS += -L$$shell_path($$PYLON_DEV_DIR/lib) -lpylonbase \
            -lpylonutility \
            -lGenApi_gcc_v3_1_Basler_pylon \
            -lGCBase_gcc_v3_1_Basler_pylon \
            -L$$shell_path(/usr/local/lib) -lopencv_core \
            -lopencv_highgui \
            -lopencv_imgproc
}
win32 {
    !contains(QMAKE_TARGET.arch, x86_64) {
        message("x86 build")
        LIBS += -L$$(PYLON_DEV_DIR)/lib/Win32
    } else {
        message("x86_64 build")
        LIBS += -L$$(PYLON_DEV_DIR)/lib/x64
    }
}
message($$PYLON_DEV_DIR)

SOURCES += \
        QPylonCamera.cpp \
        main.cpp \
        qimagepainter.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    QPylonCamera.h \
    qimagepainter.h
