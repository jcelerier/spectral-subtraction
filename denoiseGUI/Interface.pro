#-------------------------------------------------
#
# Project created by QtCreator 2013-06-05T16:30:46
#
#-------------------------------------------------

QT       += core gui multimedia widgets
DESTDIR = $$PWD/../output

TARGET = Interface
TEMPLATE = app
CONFIG += c++11
QMAKE_CXX = g++
QMAKE_CXXFLAGS_RELEASE += -fopenmp -O3 -march=native -D_GLIBCXX_PARALLEL
QMAKE_LFLAGS_RELEASE += -fopenmp

BASEPATH = ../build/denoiseGUI


CONFIG(debug, debug|release) {
	BUILDDIR = $${BASEPATH}/debug
} else {
	BUILDDIR = $${BASEPATH}/release
}
OBJECTS_DIR = $${BUILDDIR}/obj
MOC_DIR = $${BUILDDIR}/moc
RCC_DIR = $${BUILDDIR}/rcc
UI_DIR = $${BUILDDIR}/ui


SOURCES += main.cpp\
		mainwindow.cpp \
	filemanager.cpp \
	audiomanager.cpp \
	dataholder.cpp \
	batchprocessing.cpp \
	batchfileprocessing.cpp

HEADERS  += mainwindow.h \
	filemanager.h \
	audiomanager.h \
	dataholder.h \
	batchprocessing.h \
	batchfileprocessing.h \



FORMS    += mainwindow.ui \
	batchprocessing.ui \
	batchfileprocessing.ui


INCLUDEPATH += $$PWD/../libnoisered $$PWD/../libnoisered/learning

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../output/ -lnoisered
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../output/ -lnoisered
else:unix:!macx: LIBS += -L$$PWD/../output/ -lnoisered

INCLUDEPATH += $$PWD/../libnoisered
DEPENDPATH += $$PWD/../libnoisered

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../output/noisered.lib
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../output/noisered.lib
else:unix:!macx: PRE_TARGETDEPS += $$PWD/../output/libnoisered.a
LIBS += -lfftw3  -lcwt
