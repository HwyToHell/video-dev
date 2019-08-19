QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = trace
TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS
CONFIG += c++11

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
        ../../car-count/src/tracker.cpp \
        ../../utilities/src/util-visual-trace.cpp \
        ../src/main.cpp \
        ../src/mainwindow.cpp \
        ../src/sql_trace.cpp \
        ../src/trackimages.cpp

HEADERS += \
        ../../../cpp/inc/id_pool.h \
        ../../../cpp/inc/program_options.h \
        ../../car-count/include/config.h \
        ../../car-count/include/frame_handler.h \
        ../../car-count/include/tracker.h \
        ../../utilities/inc/util-visual-trace.h \
        ../inc/mainwindow.h \
        ../inc/sql_trace.h \
        ../inc/trackimages.h

FORMS += \
    ../frm/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
