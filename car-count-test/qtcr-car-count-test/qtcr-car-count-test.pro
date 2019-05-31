TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -Wno-unknown-pragmas # suppress warning emitted by catch 1.3.5


unix: {
CONFIG += link_pkgconfig
PKGCONFIG += sqlite3

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
}


win32: {
INCLUDEPATH += $$(BOOST)
INCLUDEPATH += D:/opencv-3.4.0-mingw/include
INCLUDEPATH += D:/Holger/app-dev/sqlite/inc
INCLUDEPATH += D:/Holger/app-dev/catch

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
    ../../../cpp/src/id_pool.cpp \
    ../../../cpp/src/pick_list.cpp \
    ../../../cpp/src/program_options.cpp \
    ../../car-count/src/config.cpp \
    ../../car-count/src/recorder.cpp \
    ../../car-count/src/tracker.cpp \
    ../../utilities/src/util-visual-trace.cpp \
    ../src/main.cpp \
    ../src/occlusion_test_v1.cpp \
    ../src/program_options_test.cpp \
    ../src/scene_tracker_test_v1.cpp \
    ../src/track_test_v1.cpp

HEADERS += \
    ../../../cpp/inc/id_pool.h \
    ../../../cpp/inc/observer.h \
    ../../../cpp/inc/pick_list.h \
    ../../../cpp/inc/program_options.h \
    ../../../cpp/inc/rlutil.h \
    ../../car-count/include/config.h \
    ../../car-count/include/recorder.h \
    ../../car-count/include/tracker.h \
    ../../utilities/inc/util-visual-trace.h \
    ../src/stdafx.h
