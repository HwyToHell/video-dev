QT       += core gui
QT += widgets

TARGET = trace-test
TEMPLATE = app
CONFIG += c++14


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
}

SOURCES += \
        ../../../cpp/src/id_pool.cpp \
        ../../../cpp/src/program_options.cpp \
        ../../car-count/src/config.cpp \
        ../../car-count/src/frame_handler.cpp \
        ../../car-count/src/tracker.cpp \
        ../../utilities/src/util-visual-trace.cpp \
        ../src/sql-main.cpp \
        ../src/sql-trace-test.cpp \
        ../src/sql_trace.cpp \
        ../src/trackimages.cpp


HEADERS += \
        ../../../cpp/inc/id_pool.h \
        ../../../cpp/inc/program_options.h \
        ../../car-count/include/config.h \
        ../../car-count/include/frame_handler.h \
        ../../car-count/include/tracker.h \
        ../../utilities/inc/util-visual-trace.h \
        ../inc/sql_trace.h \
        ../inc/trackimages.h

