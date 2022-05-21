QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 -framework OpenCL

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
QMAKE_LFLAGS += -framework OpenCL
SOURCES += \
    jsonscenereader.cpp \
    main.cpp \
    mainwindow.cpp \
    openclraytracerharness.cpp \
    opencltypes.cpp \
    raytracer.cpp \
    scene.cpp

HEADERS += \
    jsonscenereader.h \
    mainwindow.h \
    openclraytracerharness.h \
    opencltypes.h \
    raytracer.h \
    scene.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    kernels.qrc

INCLUDEPATH += include
 DEFINES += GLM_FORCE_RADIANS
