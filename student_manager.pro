QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = student_manager
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    studentdialog.cpp \
    student.cpp

HEADERS += \
    mainwindow.h \
    studentdialog.h \
    student.h

DEFINES += QT_DEPRECATED_WARNINGS

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
