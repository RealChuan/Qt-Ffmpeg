include(../../common.pri)

QT       += core gui widgets network multimedia openglwidgets core5compat

TEMPLATE = app

TARGET = MpvPlayer

DEFINES += MPV_ON

LIBS += \
    -l$$replaceLibName(custommpv) \
    -l$$replaceLibName(thirdparty) \
    -l$$replaceLibName(dump) \
    -l$$replaceLibName(utils)

include(../../src/3rdparty/3rdparty.pri)
include(../../src/mpv/mpv.pri)

SOURCES += \
    ../common/controlwidget.cc \
    ../common/openwebmediadialog.cc \
    ../common/playlistmodel.cpp \
    ../common/playlistview.cc \
    ../common/qmediaplaylist.cpp \
    ../common/qplaylistfileparser.cpp \
    ../common/slider.cpp \
    ../common/titlewidget.cc \
    main.cc \
    mainwindow.cc \
    mpvlogwindow.cc \
    subtitledelaydialog.cc

HEADERS += \
    ../common/controlwidget.hpp \
    ../common/openwebmediadialog.hpp \
    ../common/playlistmodel.h \
    ../common/playlistview.hpp \
    ../common/qmediaplaylist.h \
    ../common/qmediaplaylist_p.h \
    ../common/qplaylistfileparser_p.h \
    ../common/slider.h \
    ../common/titlewidget.hpp \
    mainwindow.hpp \
    mpvlogwindow.hpp \
    subtitledelaydialog.hpp

DESTDIR = $$APP_OUTPUT_PATH

win32{
    QMAKE_POST_LINK += $$QMAKE_COPY_FILE C:\\3rd\\x64\\mpv\\libmpv-2.dll $$replace(APP_OUTPUT_PATH, /, \\)\\libmpv-2.dll
}
