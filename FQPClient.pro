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
    libs.files = $$OUT_PWD/libFQPClient.a
    INSTALLS += headers
    INSTALLS += libs
}

macx {
    CONFIG += shared
    #CONFIG += staticlib
    FRAMEWORK_HEADERS.version = Versions
}

ios {
    message("ios")
    #CONFIG += shared
    CONFIG += staticlib
    CONFIG += framework

    QMAKE_IOS_DEPLOYMENT_TARGET = 10
    QMAKE_FRAMEWORK_BUNDLE_NAME = $$LIBRARY_NAME
}

macx|ios: {
    CONFIG += lib_bundle
    # FRAMEWORK_HEADERS.files = $${HEADERS}
    # FRAMEWORK_HEADERS.path = Headers
    # QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS

    # If we're static but a bundle, we want to copy files to a framework
    # directory.
    CONFIG(staticlib, shared|staticlib){
        message("Adding copy for static framework")
        # (using QMAKE_EXTRA_TARGET will be executed before linking, which is
        # too early).
        QMAKE_POST_LINK += mkdir -p $${TARGET}.framework/Headers && \
            $$QMAKE_COPY $$PWD/*.h $${TARGET}.framework/Headers && \
            $$QMAKE_COPY $$OUT_PWD/lib$${TARGET}.a $${TARGET}.framework/$${TARGET} && \
            $$QMAKE_RANLIB -s $${TARGET}.framework/$${TARGET}
    }
}

DISTFILES += \
    Readme.md
