QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

# Определить операционную систему
win32 {
    DEFINES += _WINDOWS
    LIBS += -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -lshlwapi -lole32
}

TARGET = student_manager

SOURCES += main.cpp

# Для Windows подсистема windows (без консоли)
win32:QMAKE_LFLAGS += -mwindows
