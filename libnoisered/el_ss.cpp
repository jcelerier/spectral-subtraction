#include "el_ss.h"
#include <cmath>
#include <fstream>
#include <clocale>

EqualLoudnessSpectralSubtraction::EqualLoudnessSpectralSubtraction(SubtractionConfiguration &configuration):
	SimpleSpectralSubtraction(configuration)
{
	algorithm = Algorithm::EqualLoudness;
}


void EqualLoudnessSpectralSubtraction::operator()(fftw_complex *input_spectrum, double *noise_spectrum)
{
	for (auto i = 0U; i < conf.spectrumSize(); ++i)
	{
		double Apower, Bpower, magnitude, phase, y, alpha, beta;
		alpha = _alpha - _alphawt * (loudness_contour[i] - 60);
		beta  = _beta  - _betawt  * (loudness_contour[i] - 60);

		y = std::pow(input_spectrum[i][0], 2) + std::pow(input_spectrum[i][1], 2);

		Apower = y - alpha * noise_spectrum[i];
		Bpower = beta * y;

		magnitude = std::sqrt((Apower > Bpower) ? Apower : Bpower);
		phase = std::atan2(input_spectrum[i][1], input_spectrum[i][0]);

		input_spectrum[i][0] = magnitude * std::cos(phase);
		input_spectrum[i][1] = magnitude * std::sin(phase);
	}
}

void EqualLoudnessSpectralSubtraction::prepare()
{
	loadLoudnessContour();
}

EqualLoudnessSpectralSubtraction::~EqualLoudnessSpectralSubtraction()
{
	delete[] loudness_contour;
}


// TODO C++ PLIZ
void EqualLoudnessSpectralSubtraction::loadLoudnessContour()
{
	// loading data for loudness contour algo
#ifdef __linux__
	setlocale(LC_ALL, "POSIX");
	// Because on french OS linux will try to read numbers with commas instead of dots
#endif

	// NOTE : the loudnes_xxx (ex. : xxx = 512)
	// refers to a spectrum with symmetrical coefficients
	// but fftw only compute non-symmetric part, so we only have to read one half of the file.
	// We choose to read the first half, hence it is in reverse order.

	char path[30];
	sprintf(path, "60phon/loudness_%d.data", conf.FFTSize());

	std::ifstream ldata(path);

	if(loudness_contour != nullptr) delete[] loudness_contour;
	loudness_contour = new double[conf.FFTSize() / 2];

	for (auto i = 0U; i < conf.FFTSize() / 2; ++i)
	{
		ldata >> loudness_contour[(conf.FFTSize() / 2 - 1) - i];
	}
	ldata.close();
}



double EqualLoudnessSpectralSubtraction::alphawt() const
{
	return _alphawt;
}

void EqualLoudnessSpectralSubtraction::setAlphawt(double value)
{
	_alphawt = std::max(value, 0.000001);
}
double EqualLoudnessSpectralSubtraction::betawt() const
{
	return _betawt;
}

void EqualLoudnessSpectralSubtraction::setBetawt(double value)
{
	_betawt = std::max(value, 0.000001);
}