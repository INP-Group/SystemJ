######################################################################

TEMPLATE = lib
CONFIG += qt warn_on exceptions release dll staticlib
QT -= gui
QMAKE_CFLAGS_DEBUG += -fPIC
QMAKE_CFLAGS_RELEASE += -fPIC

TARGET = Qcxscheduler
HEADERS += Qcxscheduler.h
SOURCES += Qcxscheduler.cpp

INCLUDEPATH += $${TOP_INCLUDES}
