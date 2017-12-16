#-------------------------------------------------
#
# Project created by QtCreator 2017-05-07T10:12:41
#
#-------------------------------------------------

QT       += network
QT       -= gui

CONFIG += c++11

TARGET = FQPClient
TEMPLATE = lib

DEFINES += FQPCLIENT_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    FQPClient.cpp \
    FQPReplyHandler.cpp \
    FQPRequest.cpp \

HEADERS +=\
        fqpclient_global.h \
        FQPClient.h \
        FQPTypes.h \
        FQPReplyHandler.h \
        FQPRequest.h \

android {
    CONFIG -= shared
    CONFIG += static

    headers.path = $$OUT_PWD/../include
    headers.files = $$HEADERS
    libs.path = $$OUT_PWD/../lib
    # The ./ is necessary, or we'll copy from the location of the .pro file.
    libs.files = ./libFQPClient.a
    INSTALLS += headers
    INSTALLS += libs
}

macx|ios: {
    message("OS X and iOS")
    CONFIG += shared
    CONFIG += lib_bundle
    FRAMEWORK_HEADERS.version = Versions
    FRAMEWORK_HEADERS.files = $${HEADERS}
    FRAMEWORK_HEADERS.path = Headers
    QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
}

DISTFILES += \
    Readme.md
