TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11
#unix: CONFIG += link_pkgconfig
#unix: PKGCONFIG += opencv
unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += sqlite3

INCLUDEPATH += /usr/local/include
QMAKE_LIBDIR += /usr/local/lib

# static libs, watch correct order!
LIBS += -lopencv_highgui
LIBS += -lopencv_video
LIBS += -lopencv_imgproc
LIBS += -lopencv_core

# dynamic libs
LIBS += -lv4l2 -lv4l1
LIBS += -lgstvideo-0.10 -lgstapp-0.10
LIBS += -lxml2
LIBS += -lglib-2.0
LIBS += -lgthread-2.0
LIBS += -lgobject-2.0
LIBS += -lgstreamer-0.10 -lgstbase-0.10
LIBS += -ltiff -lpng -ljpeg
LIBS += -lQt5Core
LIBS += -lQt5Gui
LIBS += -lQt5OpenGL
LIBS += -lQt5Test
LIBS += -lQt5Widgets
LIBS += -lXext -lX11
LIBS += -lICE -lSM -lGL
LIBS += -lrt -lpthread
LIBS += -lm -ldl
LIBS += -lstdc++
LIBS += -lz

SOURCES += \
    ../src/config.cpp \
    ../src/frame_handler.cpp \
    ../src/recorder.cpp \
    ../src/tracker.cpp \
    ../src/video.cpp \
    ../../cpp/src/program_options.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    ../include/config.h \
    ../include/frame_handler.h \
    ../include/recorder.h \
    ../include/tracker.h \
    ../../cpp/inc/observer.h \
    ../../cpp/inc/program_options.h

