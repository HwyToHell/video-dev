TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11
#unix: CONFIG += link_pkgconfig
#unix: PKGCONFIG += opencv
unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += sqlite3

INCLUDEPATH += /home/holger/app-dev/Catch2/single_include/catch2
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
    ../../car-count/src/config.cpp \
    ../../car-count/src/frame_handler.cpp \
    ../../car-count/src/recorder.cpp \
    ../../car-count/src/tracker.cpp \
    ../src/config_test.cpp \
    ../src/frame_handler_test.cpp \
    ../src/main.cpp \
    ../src/program_options_test.cpp \
    ../src/scene_tracker_test.cpp \
    ../src/track_test.cpp \
    ../../cpp/src/program_options.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    ../../car-count/include/config.h \
    ../../car-count/include/frame_handler.h \
    ../../car-count/include/recorder.h \
    ../../car-count/include/tracker.h \
    ../../cpp/inc/observer.h \
    ../../cpp/inc/program_options.h

