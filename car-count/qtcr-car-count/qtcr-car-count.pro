TEMPLATE = app
TARGET = counter

CONFIG += console c++11
CONFIG -= app_bundle qt


linux {
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
    ../../trace/src/sql_trace.cpp \
    ../../utilities/src/util-visual-trace.cpp \
    ../src/config.cpp \
    ../src/frame_handler.cpp \
    ../src/recorder.cpp \
    ../src/tracker.cpp \
    ../src/video.cpp


HEADERS += \
    ../../../cpp/inc/id_pool.h \
    ../../../cpp/inc/observer.h \
    ../../../cpp/inc/program_options.h \
    ../../trace/inc/sql_trace.h \
    ../../utilities/inc/util-visual-trace.h \
    ../include/config.h \
    ../include/frame_handler.h \
    ../include/recorder.h \
    ../include/tracker.h


