#-------------------------------------------------
#
# Project created by QtCreator 2016-01-28T09:38:55
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Sampler_NT1065
TEMPLATE = app

LIBS += ..\source\library\lib\x86\CyAPI.lib
LIBS += /NODEFAULTLIB:LIBCMT
LIBS += "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\User32.lib"
LIBS += "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib\setupapi.lib"

SOURCES += main.cpp\
        mainwindow.cpp \
    cy3device.cpp

HEADERS  += mainwindow.h \
    cy3device.h

FORMS    += mainwindow.ui