QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = trace
TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS
CONFIG += c++11

win32: {
INCLUDEPATH += $$(BOOST)
INCLUDEPATH += D:/opencv-3.4.0-mingw/include
INCLUDEPATH += D:/Holger/app-dev/sqlite/inc

LIBS += -L$$(BOOST)/lib \
    -lboost_filesystem-mgw73-mt-d-x32-1_67 \
    -lboost_system-mgw73-mt-d-x32-1_67
LIBS += -LD:/opencv-3.4.0-mingw/bin \
    -lopencv_core340 \
    -lopencv_highgui340 \
    -lopencv_imgcodecs340 \
    -lopencv_imgproc340 \
    -lopencv_video340 \
    -lopencv_videoio340
LIBS += D:/Holger/app-dev/sqlite/bin/sqlite3.dll
}

SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
