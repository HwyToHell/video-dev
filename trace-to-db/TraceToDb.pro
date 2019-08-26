#-------------------------------------------------
#
# Project created by QtCreator 2019-08-19T12:57:37
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TraceToDb
TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS
CONFIG += c++11

LIBS += -lboost_filesystem \
        -lboost_system
LIBS += -lopencv_core \
        -lopencv_imgcodecs \
        -lopencv_imgproc \
        -lopencv_highgui \
        -lopencv_video \
        -lopencv_videoio
LIBS += -lsqlite3

SOURCES += \
    ../../cpp/src/id_pool.cpp \
    ../../cpp/src/program_options.cpp \
    ../car-count/src/config.cpp \
    ../car-count/src/tracker.cpp \
    ../trace/src/sql_trace.cpp \
    ../trace/src/trackimages.cpp \
    ../utilities/src/util-visual-trace.cpp \
    clickablelabel.cpp \
        src/main.cpp \
        src/tracetodb.cpp \

HEADERS += \
    ../../cpp/inc/id_pool.h \
    ../../cpp/inc/program_options.h \
    ../car-count/include/config.h \
    ../car-count/include/frame_handler.h \
    ../car-count/include/tracker.h \
    ../trace/inc/sql_trace.h \
    ../trace/inc/trackimages.h \
    ../utilities/inc/util-visual-trace.h \
    clickablelabel.h \
        inc/tracetodb.h \

FORMS += \
        frm/tracetodb.ui \

