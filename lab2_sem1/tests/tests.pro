QT += widgets
CONFIG += console
greaterThan(QT_MAJOR_VERSION, 5) {
    CONFIG += c++17
} else {
    CONFIG += c++11
}
CONFIG -= app_bundle
TEMPLATE = app
TARGET = calculator_tests
INCLUDEPATH += ..
SOURCES += tests.cpp ../calculation.cpp ../calculator.cpp
HEADERS += ../calculation.h ../calculator.h
