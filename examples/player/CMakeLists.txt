set(PROJECT_SOURCES
    colorspacedialog.cc
    colorspacedialog.hpp
    controlwidget.cc
    controlwidget.hpp
    main.cpp
    mainwindow.cpp
    mainwindow.h
    openwebmediadialog.cc
    openwebmediadialog.hpp
    playlistmodel.cpp
    playlistmodel.h
    playlistview.cc
    playlistview.hpp
    qmediaplaylist_p.h
    qmediaplaylist.cpp
    qmediaplaylist.h
    qplaylistfileparser_p.h
    qplaylistfileparser.cpp
    slider.cpp
    slider.h
    titlewidget.cc
    titlewidget.hpp)

qt_add_executable(Player MANUAL_FINALIZATION ${PROJECT_SOURCES})
target_link_libraries(Player PRIVATE ffmpeg thirdparty utils Qt6::Widgets
                                     Qt6::Multimedia Qt6::OpenGLWidgets)
target_link_libraries(Player PRIVATE PkgConfig::ffmpeg)
if(CMAKE_HOST_APPLE)
  target_link_libraries(
    Player
    PRIVATE ${Foundation_LIBRARY}
            ${CoreAudio_LIBRARY}
            ${AVFoundation_LIBRARY}
            ${CoreGraphics_LIBRARY}
            ${OpenGL_LIBRARY}
            ${CoreText_LIBRARY}
            ${CoreImage_LIBRARY}
            ${AppKit_LIBRARY}
            ${Security_LIBRARY}
            ${AudioToolBox_LIBRARY}
            ${VideoToolBox_LIBRARY}
            ${CoreFoundation_LIBRARY}
            ${CoreMedia_LIBRARY}
            ${CoreVideo_LIBRARY}
            ${CoreServices_LIBRARY})
endif()
qt_finalize_executable(Player)
