set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        openwebmediadialog.cc openwebmediadialog.hpp
        slider.cpp slider.h)

qt_add_executable(QFfmpegPlayer MANUAL_FINALIZATION ${PROJECT_SOURCES})
target_link_libraries(QFfmpegPlayer PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::OpenGLWidgets 
    shared_qtsingleapplication crashhandler ffmpeg utils)
target_link_libraries(QFfmpegPlayer PRIVATE PkgConfig::ffmpeg) 

if (CMAKE_HOST_APPLE)
  target_link_libraries(QFfmpegPlayer PRIVATE ${Foundation_LIBRARY} ${CoreAudio_LIBRARY} ${AVFoundation_LIBRARY} ${CoreGraphics_LIBRARY} ${OpenGL_LIBRARY} ${CoreText_LIBRARY} ${CoreImage_LIBRARY} ${AppKit_LIBRARY} ${Security_LIBRARY} ${AudioToolBox_LIBRARY} ${VideoToolBox_LIBRARY} ${CoreFoundation_LIBRARY} ${CoreMedia_LIBRARY} ${CoreVideo_LIBRARY} ${CoreServices_LIBRARY})
endif ()

qt_finalize_executable(QFfmpegPlayer)