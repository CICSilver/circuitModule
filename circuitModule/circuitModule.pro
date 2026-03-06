# QMake project file for Qt 4.8 + GCC 4.9.2 (MinGW) build

TEMPLATE = lib
TARGET = circuitModule
DESTDIR = $$PWD/bin
# Qt modules (Qt4 style)
QT += core gui svg xml

# Warnings and RTTI/exceptions (defaults in most mkspecs, kept explicit)
CONFIG += warn_on exceptions rtti dll
DEFINES += CIRCUITMODULE_LIBRARY

# Generated code/output directories
MOC_DIR = GeneratedFiles
UI_DIR  = GeneratedFiles
RCC_DIR = GeneratedFiles
OBJECTS_DIR = build

# Make sure generated dirs are in include search path
INCLUDEPATH += $$PWD \
               $$PWD/include \
               $$PWD/include/pugixml \
               $$PWD/../../ysd_rtdb_dll \
               $$MOC_DIR $$UI_DIR $$RCC_DIR

# Headers (for IDE convenience; moc will pick up those with Q_OBJECT)
HEADERS += \
    basemodel.h \
    circuitconfig.h \
    InteractiveSvgItem.h \
    mainwindow.h \
    PainterStateGuard.h \
    RtdbClient.h \
    svgmodel.h \
    SvgTransformer.h \
    SvgUtils.h \
    CircuitModuleApi.h \
    circuitmodule_global.h \
    directitems.h \
    directwidget.h \
    include/cime/cime.h \
    include/rtdb/rtdb_dll_def.h \
    include/rtdb/rtdb_dll.h \
    include/rtdb/YsdRtdbDefine.h \
    include/rtdb/YsdRtdbEle.h \
    include/rtdb/YsdRtdbInclude.h

# UI forms
FORMS += \
    mainwindow.ui \

# Resources
RESOURCES += \
    mainwindow.qrc

# Library sources
SOURCES += \
    circuitconfig.cpp \
    InteractiveSvgItem.cpp \
    CircuitModuleApi.cpp \
    directitems.cpp \
    directwidget.cpp \
    mainwindow.cpp \
    RtdbClient.cpp \
    SvgTransformer.cpp \
    SvgUtils.cpp \
    include/pugixml/pugixml.cpp \
    include/rtdb/rtdb_dll.cpp

# YSD RTDB SDK (bundled source in repo)
SOURCES += \
    ../../ysd_rtdb_include/YsdRtdbAccess.cpp \
    ../../ysd_rtdb_include/YsdRtdbData.cpp \
    ../../ysd_rtdb_include/YsdRtdbEle.cpp \
    ../../ysd_rtdb_include/YsdRtdbModel.cpp \
    ../../ysd_rtdb_include/YsdRtdbPub.cpp \
    ../../ysd_rtdb_include/RtdbShm.cpp \
    ../../ysd_rtdb_include/YsdShm.cpp

# On Windows/MinGW, build as GUI app (no console window)
win32:CONFIG -= console

# Optional: tune linker flags if needed
# win32:LIBS += -lws2_32 -lwinmm -liphlpapi
