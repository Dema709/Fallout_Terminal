QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Common/QTextTableWidget.cpp \
    Fallout_Terminal/Fallout_Terminal.cpp \
    main.cpp

HEADERS += \
    Common/QTextTableWidget.h \
    Common/random.hpp \
    Fallout_Terminal/Fallout_Terminal.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#Работает при добавлении для сборки параметра make: install
#install_it.path = $$OUT_PWD
#install_it.files = Fallout_Terminal/litw-win.txt
#INSTALLS += install_it

RESOURCES += \
    res.qrc

ANDROID_ABIS = armeabi-v7a arm64-v8a x86 x86_64

QT += androidextras
ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
OTHER_FILES += android/AndroidManifest.xml
