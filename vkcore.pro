#-------------------------------------------------
#
# Project created by QtCreator 2015-05-16T19:46:19
#
#-------------------------------------------------

QT       += core gui network webkitwidgets sql






TARGET = vkcore
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    helpers.cpp \
    account.cpp \
    task.cpp \
    core.cpp

MYSQL_LIB=/usr/local/mysql/lib

HEADERS  += mainwindow.h \
    helpers.h \
    account.h \
    task.h \
    core.h

FORMS    += mainwindow.ui
