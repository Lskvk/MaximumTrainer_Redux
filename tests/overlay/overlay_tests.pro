###############################################################################
# tests/overlay/overlay_tests.pro
#
# Standalone Qt Test project for the in-game WorkoutOverlay helpers.
# overlayzones.h is header-only and depends only on Qt Core + Gui.
#
# Build:
#   qmake overlay_tests.pro && make
# Run:
#   ../../build/tests/overlay_tests -v2
###############################################################################

QT       += core gui testlib

CONFIG   += qt c++17 console
CONFIG   -= app_bundle

TARGET   = overlay_tests
TEMPLATE = app

DESTDIR  = ../../build/tests

INCLUDEPATH += ../../src/ui/components

HEADERS += ../../src/ui/components/overlayzones.h
SOURCES += tst_overlay.cpp
