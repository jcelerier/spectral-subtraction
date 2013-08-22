QT       -= core gui

TARGET = noisered
TEMPLATE = lib
CONFIG += staticlib c++11 openmp
DESTDIR = $$PWD/../output
LIBS += -lcwt -lfftw3

msvc:QMAKE_CXXFLAGS_RELEASE += -openmp -arch:AVX
else:QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -fopenmp -D_GLIBCXX_PARALLEL

BASEPATH = ../build/libnoisered

CONFIG(debug, debug|release) {
	BUILDDIR = $${BASEPATH}/debug
} else {
	BUILDDIR = $${BASEPATH}/release
}
OBJECTS_DIR = $${BUILDDIR}/obj
MOC_DIR = $${BUILDDIR}/moc
RCC_DIR = $${BUILDDIR}/rcc
UI_DIR = $${BUILDDIR}/ui



SOURCES += \
	estimation/wavelets/point.cpp \
	estimation/wavelets/pair.cpp \
	estimation/wavelets/matrix.cpp \
	eval.cpp \
	estimation/wavelets/cwt_noise_estimator.cpp \
	estimation/wavelets/area.cpp \
	subtraction/simple_ss.cpp \
	subtraction/el_ss.cpp \
	subtraction/geometric_ss.cpp \
	estimation/estimation_algorithm.cpp \
	subtraction/subtraction_algorithm.cpp \
	estimation/simple_estimation.cpp \
	estimation/martin_estimation.cpp \
	estimation/wavelet_estimation.cpp \
	subtraction_manager.cpp \
	mathutils/math_util.cpp \
	fft/fftmanager.cpp \
	fft/fftwmanager.cpp

HEADERS += \
	estimation/wavelets/point.h \
	estimation/wavelets/pair.h \
	estimation/wavelets/matrix.h \
	eval.h \
	estimation/wavelets/cwt_noise_estimator.h \
	estimation/wavelets/area.h \
	subtraction/simple_ss.h \
	subtraction/el_ss.h \
	subtraction/geometric_ss.h \
	estimation/estimation_algorithm.h \
	subtraction/subtraction_algorithm.h \
	estimation/simple_estimation.h \
	estimation/martin_estimation.h \
	estimation/wavelet_estimation.h \
	subtraction/algorithms.h \
	estimation/algorithms.h \
	mathutils/spline.hpp \
	subtraction_manager.h \
	mathutils/math_util.h \
	fft/fftmanager.h \
	fft/fftwmanager.h

#Learning:
#SOURCES += \
#	learning/standardssstate.cpp \
#	learning/standardssaction.cpp \
#	learning/sarsa.cpp \
#	learning/realaction.cpp \
#	learning/learning.cpp \
#	subtraction/learning_ss.cpp

#HEADERS += \
#	learning/state.hpp \
#	learning/standardssstate.hpp \
#	learning/standardssaction.hpp \
#	learning/learning.hpp \
#	learning/sarsa.hpp \
#	learning/realstate.hpp \
#	learning/realaction.hpp \
#	learning/action.hpp \
#	subtraction/learning_ss.h

