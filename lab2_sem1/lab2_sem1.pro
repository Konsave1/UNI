QT += widgets
greaterThan(QT_MAJOR_VERSION, 5) {
    CONFIG += c++17
} else {
    CONFIG += c++11
}
TEMPLATE = app
TARGET = lab2_sem1
SOURCES += main.cpp calculation.cpp calculator.cpp
HEADERS += calculation.h calculator.h
