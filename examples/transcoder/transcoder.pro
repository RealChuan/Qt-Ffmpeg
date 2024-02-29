include(../../Common.pri)

QT       += core gui widgets network multimedia openglwidgets core5compat

TEMPLATE = app

TARGET = Transcoder

LIBS += \
    -L$$APP_OUTPUT_PATH/../libs \
    -l$$replaceLibName(ffmpeg) \
    -l$$replaceLibName(thirdparty) \
    -l$$replaceLibName(utils)

include(../../3rdparty/3rdparty.pri)

SOURCES += \
    audioencoderwidget.cc \
    main.cc \
    mainwindow.cc \
    outputwidget.cc \
    previewwidget.cc \
    sourcewidget.cc \
    stautuswidget.cc \
    videoencoderwidget.cc

HEADERS += \
    audioencoderwidget.hpp \
    mainwindow.hpp \
    outputwidget.hpp \
    previewwidget.hpp \
    sourcewidget.hpp \
    stautuswidget.hpp \
    videoencoderwidget.hpp

DESTDIR = $$APP_OUTPUT_PATH

DEFINES += QT_DEPRECATED_WARNINGS
