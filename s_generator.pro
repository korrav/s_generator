#-------------------------------------------------
#
# Project created by QtCreator 2014-10-27T09:44:50
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = s_generator
TEMPLATE = app

CONFIG +=console
CONFIG += c++11

SOURCES += main.cpp\
        gwidget.cpp

HEADERS  += gwidget.h \
    samples.h

RESOURCES += \
    resource.qrc
