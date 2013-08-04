#include "subtractionconfiguration.h"
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>

#include <climits>

SubtractionConfiguration::SubtractionConfiguration(int fft_Size, int sampling_Rate):
	_fftSize(fft_Size),
	_samplingRate(sampling_Rate),
	_data(nullptr),
	_origData(nullptr),
	_tabLength(0),
	_useOLA(false),
	_bypass(false)
{
	onFFTSizeUpdate();
}





void SubtractionConfiguration::onFFTSizeUpdate()
{
	_spectrumSize = _fftSize / 2 + 1;
	ola_frame_increment = _fftSize / 2;
	frame_increment = _fftSize;

	in = fftw_alloc_real(_fftSize);
	out = fftw_alloc_real(_fftSize);

	_spectrum = fftw_alloc_complex(_spectrumSize);


	// Initialize the fftw plans (so.. FAAAST)
	plan_fw = fftw_plan_dft_r2c_1d(_fftSize, in, _spectrum, FFTW_ESTIMATE);
	plan_bw = fftw_plan_dft_c2r_1d(_fftSize, _spectrum, out, FFTW_ESTIMATE);

	if(estimation) estimation->onFFTSizeUpdate();
	if(subtraction) subtraction->onFFTSizeUpdate();
}

double SubtractionConfiguration::ShortToDouble(short x)
{
	static const double normalizationFactor = 1.0 / pow(2.0, sizeof(short) * 8 - 1.0);
	return x * normalizationFactor;
}

short SubtractionConfiguration::DoubleToShort(double x)
{
	static const double denormalizationFactor = pow(2.0, sizeof(short) * 8 - 1.0);
	return x * denormalizationFactor;
}

void SubtractionConfiguration::copyInput(unsigned int pos)
{
	if(_useOLA)
		copyInputOLA(pos);
	else
		copyInputSimple(pos);
}

void SubtractionConfiguration::copyOutput(unsigned int pos)
{
	if(_useOLA)
		copyOutputOLA(pos);
	else
		copyOutputSimple(pos);
}

void SubtractionConfiguration::clean()
{
	fftw_free(in);
	fftw_free(out);

	fftw_free(_spectrum);
}

SubtractionConfiguration::~SubtractionConfiguration()
{
	clean();

	delete[] _data;
	delete[] _origData;
}

void SubtractionConfiguration::initDataArray()
{
	std::copy_n(_origData, _tabLength, _data);
}

unsigned int SubtractionConfiguration::iterations() const
{
	return _iterations;
}

void SubtractionConfiguration::setIterations(int value)
{
	_iterations = std::max(value, 1);
}


double *SubtractionConfiguration::getData()
{
	return _data;
}

void SubtractionConfiguration::onDataUpdate()
{
	estimation->onDataUpdate();
	subtraction->onDataUpdate();
}

double *SubtractionConfiguration::getNoisyData()
{
	return _origData;
}

unsigned int SubtractionConfiguration::getLength()
{
	return _tabLength;
}

unsigned int SubtractionConfiguration::readFile(char *str)
{
	std::ifstream ifile(str, std::ios_base::ate | std::ios_base::binary);
	_tabLength = ifile.tellg() / (sizeof(short) / sizeof(char));
	ifile.clear();
	ifile.seekg(0, std::ios_base::beg);

	delete[] _origData;
	delete[] _data;
	_origData = new double[_tabLength];
	_data = new double[_tabLength];

	// We have to get signal between -1 and 1
	static const double normalizationFactor = pow(2.0, sizeof(short) * 8 - 1.0);

	unsigned int pos = 0;
	short sample;
	while (ifile.read((char *)&sample, sizeof(short)) && pos < _tabLength)
	{
		_origData[pos++] = sample / normalizationFactor;
	}

	ifile.close();
	_dataSource = DataSource::File;
	return _tabLength;
}

unsigned int SubtractionConfiguration::readBuffer(short *buffer, int length)
{
	if(_bypass) return length;

	_tabLength = length;
	delete[] _origData;
	delete[] _data;
	_origData = new double[_tabLength];
	_data = new double[_tabLength];

	// Julius accepts only big-endian raw files but it seems internal buffers
	// are little-endian so no need to convert.
	// std::transform(buffer, buffer + tab_length, buffer,
	//                [] (short val) {return (val << 8) | ((val >> 8) & 0xFF)});

	std::transform(buffer, buffer + _tabLength, _origData, &SubtractionConfiguration::ShortToDouble);
	initDataArray();

	_dataSource = DataSource::Buffer;
	return _tabLength;
}

void SubtractionConfiguration::writeBuffer(short *buffer)
{
	if(_bypass) return;
	std::transform(_data, _data + _tabLength, buffer, &SubtractionConfiguration::DoubleToShort);
	// Julius accepts only big-endian raw files but it seems internal buffers
	// are little-endian so no need to convert.
	// std::transform(buffer, buffer + tab_length, buffer,
	//                [] (short val) {return (val << 8) | ((val >> 8) & 0xFF)});
}

void SubtractionConfiguration::copyInputSimple(unsigned int pos)
{
	// Data copying
	if (_fftSize <= _tabLength - pos)
	{
		std::copy_n(_data + pos, _fftSize, in);
	}
	else
	{
		std::copy_n(_data + pos, _tabLength - pos, in);
		std::fill_n(in + _tabLength - pos, _fftSize - (_tabLength - pos), 0);
	}
}

void SubtractionConfiguration::copyOutputSimple(unsigned int pos)
{
	auto normalizeFFT = [&](double x) { return x / _fftSize; };
	if (_fftSize <= _tabLength - pos)
	{
		std::transform(out, out + _fftSize, _data + pos, normalizeFFT);
	}
	else //fileSize - pos < fftSize
	{
		std::transform(out, out + _tabLength - pos, _data + pos, normalizeFFT);
	}
}

void SubtractionConfiguration::copyInputOLA(unsigned int pos)
{
	// Data copying
	if (ola_frame_increment <= _tabLength - pos) // last case
	{
		std::copy_n(_data + pos, ola_frame_increment, in);
		std::fill_n(in + ola_frame_increment, ola_frame_increment, 0);

		std::fill_n(_data + pos, ola_frame_increment, 0);
	}
	else
	{
		std::copy_n(_data + pos, _tabLength - pos, in);
		std::fill_n(in + _tabLength - pos, ola_frame_increment - (_tabLength - pos), 0);

		std::fill_n(_data + pos, _tabLength - pos, 0);
	}
}

void SubtractionConfiguration::copyOutputOLA(unsigned int pos)
{
	// Lock here
	//ola_mutex.lock();
	for (auto j = 0U; (j < _fftSize) && (pos + j < _tabLength); ++j)
	{
		_data[pos + j] += out[j] / _fftSize;
	}
	// Unlock here
	//ola_mutex.unlock();
}
bool SubtractionConfiguration::OLAenabled() const
{
	return _useOLA;
}

void SubtractionConfiguration::enableOLA()
{
	_useOLA = true;
}

void SubtractionConfiguration::disableOLA()
{
	_useOLA = false;
}

void SubtractionConfiguration::setOLA(bool val)
{
	_useOLA = val;
}

void SubtractionConfiguration::forwardFFT()
{
	fftw_execute(plan_fw);
}

void SubtractionConfiguration::backwardFFT()
{
	fftw_execute(plan_bw);
}

fftw_complex *SubtractionConfiguration::spectrum()
{
	return _spectrum;
}

SubtractionConfiguration::DataSource SubtractionConfiguration::dataSource() const
{
	return _dataSource;
}

void SubtractionConfiguration::setDataSource(const SubtractionConfiguration::DataSource& val)
{
	_dataSource = val;
}

std::shared_ptr<Estimation> SubtractionConfiguration::getEstimationImplementation() const
{
	return estimation;
}

void SubtractionConfiguration::setEstimationImplementation(std::shared_ptr<Estimation> value)
{
	estimation = std::move(value);
	estimation->onFFTSizeUpdate();
}

bool SubtractionConfiguration::bypass()
{
	return _bypass;
}


unsigned int SubtractionConfiguration::getFrameIncrement()
{
	return _useOLA? ola_frame_increment : frame_increment;
}

std::shared_ptr<Subtraction> SubtractionConfiguration::getSubtractionImplementation() const
{
	return subtraction;
}

void SubtractionConfiguration::setSubtractionImplementation(std::shared_ptr<Subtraction> value)
{
	subtraction = std::move(value);
	subtraction->onFFTSizeUpdate();
}

unsigned int SubtractionConfiguration::getSamplingRate() const
{
	return _samplingRate;
}

void SubtractionConfiguration::setSamplingRate(unsigned int value)
{
	_samplingRate = value;
	clean();
	onFFTSizeUpdate();
}

/*
 * File syntax :
 * alpha
 * beta
 * alphawt
 * betawt
 * iterations
 * noise algo (std / martin / wavelets)
 * algo (std / el / ga)
*/
void SubtractionConfiguration::readParametersFromFile()
{
	std::map<std::string, std::shared_ptr<Estimation>> noise_est
	{
		std::make_pair("std", std::shared_ptr<Estimation>(new SimpleEstimation(*this))),
		std::make_pair("martin", std::shared_ptr<Estimation>(new MartinEstimation(*this))),
		std::make_pair("wavelets", std::shared_ptr<Estimation>(new WaveletEstimation(*this)))
	};

	static const std::map<std::string, Subtraction::Algorithm> algo
	{
		std::make_pair("std", Subtraction::Algorithm::Standard),
		std::make_pair("el", Subtraction::Algorithm::EqualLoudness),
		std::make_pair("ga", Subtraction::Algorithm::GeometricApproach),
		std::make_pair("bypass", Subtraction::Algorithm::Bypass)
	};

	std::ifstream f("subtraction.conf");
	double alpha, beta, alphawt, betawt;
	int iterations;
	std::string noise_alg, alg;

	// Class members
	f >> alpha;
	f >> beta;
	f >> alphawt;
	f >> betawt;
	f >> iterations;

	// Local stuff
	f >> noise_alg;
	f >> alg;
	f.close();

	setIterations(iterations);
	if (noise_est.find(noise_alg) != noise_est.end())
		setEstimationImplementation(noise_est[noise_alg]);
	else
		std::cerr << "Invalid estimation algorithm";

	if (algo.find(alg) != algo.end())
	{
		switch( algo.at(alg))
		{
			case Subtraction::Algorithm::Standard:
			{
				// Check for memory leak.
				SimpleSpectralSubtraction* subtraction = new SimpleSpectralSubtraction(*this);
				subtraction->setAlpha(alpha);
				subtraction->setBeta(beta);
				setSubtractionImplementation(std::shared_ptr<Subtraction>(subtraction));
				break;
			}
			case Subtraction::Algorithm::EqualLoudness:
			{
				EqualLoudnessSpectralSubtraction* subtraction = new EqualLoudnessSpectralSubtraction(*this);
				subtraction->setAlpha(alpha);
				subtraction->setBeta(beta);
				subtraction->setAlphawt(alphawt);
				subtraction->setBetawt(betawt);
				setSubtractionImplementation(std::shared_ptr<Subtraction>(subtraction));
				break;
			}
			case Subtraction::Algorithm::GeometricApproach:
			{
				GeometricSpectralSubtraction* subtraction = new GeometricSpectralSubtraction(*this);
				setSubtractionImplementation(std::shared_ptr<Subtraction>(subtraction));
				break;
			}
			default:
				_bypass = true;
		}
	}
	else
	{
		std::cerr << "Invalid subtraction algorithm";
	}
}

unsigned int SubtractionConfiguration::FFTSize() const
{
	return _fftSize;
}

void SubtractionConfiguration::setFftSize(unsigned int value)
{
	_fftSize = value;
	clean();
	onFFTSizeUpdate();
}

unsigned int SubtractionConfiguration::spectrumSize() const
{
	return _spectrumSize;
}


