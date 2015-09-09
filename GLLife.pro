#-------------------------------------------------
#
# Project created by QtCreator 2015-09-09T19:11:17
#
#-------------------------------------------------

QT       += core gui
QT       += opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GLLife
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    glwidget.cpp

HEADERS  += mainwindow.h \
    glwidget.h

RESOURCES += \
    files.qrc

DISTFILES +=
