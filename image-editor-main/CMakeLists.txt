cmake_minimum_required(VERSION 3.16)

project(gimp2 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts)

set(PROJECT_SOURCES
    main.cpp

    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    netpbm.cpp
    netpbm.h

    pointoperationpage.cpp
    pointoperationpage.h
    pointoperationpage.ui

    histogrampage.cpp
    histogrampage.h
    histogrampage.ui

    convolutionpage.cpp
    convolutionpage.h
    convolutionpage.ui

    binarizationpage.cpp
    binarizationpage.h
    binarizationpage.ui

    editorcontext.h

    pointoperationhandler.cpp
    pointoperationhandler.h

    histogramhandler.cpp
    histogramhandler.h

    convolutionhandler.cpp
    convolutionhandler.h
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(gimp2
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        binarizationhandler.h binarizationhandler.cpp
        cannypage.h cannypage.cpp cannypage.ui
        cannyhandler.h cannyhandler.cpp
        harrispage.h harrispage.cpp harrispage.ui
        harrishandler.h harrishandler.cpp

    )
else()
    add_executable(gimp2
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(gimp2 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
)

if(QT_VERSION VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.gimp2)
endif()

set_target_properties(gimp2 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS gimp2
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(gimp2)
endif()