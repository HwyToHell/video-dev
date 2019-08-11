TEMPLATE = app
TARGET = test

CONFIG -= app_bundle qt
CONFIG += console c++11

QMAKE_CXXFLAGS += -Wno-unknown-pragmas # suppress warning emitted by catch 1.3.5

linux {
    INCLUDEPATH += /home/holger/app-dev/catch#/catch_2.8.0

    LIBS += -lboost_filesystem \
            -lboost_system
    LIBS += -lopencv_core \
            -lopencv_imgcodecs \
            -lopencv_imgproc \
            -lopencv_highgui \
            -lopencv_video \
            -lopencv_videoio
    LIBS += -lsqlite3
    LIBS += -lv4l2 -lv4l1
}

windows {
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
    ../../../cpp/inc/program_options.h \
    ../../../cpp/inc/rlutil.h \
    ../../car-count/include/config.h \
    ../../car-count/include/recorder.h \
    ../../car-count/include/tracker.h \
    ../../utilities/inc/util-visual-trace.h \
    ../src/stdafx.h
