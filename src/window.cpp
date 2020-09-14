#include "include/window.h"

// C++
#include <iostream>

// PortAudio
#include "portaudio.h"

#include "unistd.h"

double mysquare(double t, double f, unsigned int harmonics)
{
	double s = 0.;
	for (size_t k = 1; k <= harmonics+1; ++k) {
		s += sin(2.*M_PI*(2.*k-1.)*f*t) / (2.*k-1);
	}
	s = 4./M_PI * s;
	return s;
}


/* This routine will be called by the PortAudio engine when audio is needed.
 * It may called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
 */ 
static int patestCallback( const void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData )
{
	/* Cast data passed through stream to our structure. */
	paTestData *data = (paTestData*)userData; 
	float *out = (float*)outputBuffer;
	unsigned int i;
	(void) inputBuffer; /* Prevent unused variable warning. */
	(void) timeInfo;
	(void) statusFlags;

	for( i=0; i<framesPerBuffer; i++ )
	{

		double t = data->count++ / (double)data->sr;
		*(out++) = data->sound_on * data->volume * mysquare(t, data->pitch, data->harmonics);
		*(out++) = data->sound_on * data->volume * mysquare(t, data->pitch, data->harmonics);

		if (data->collect) {
			if (data->buff_m == data->buff_n) {
				data->buff_m = 0;
				data->collect = false;
			} else {
				data->buff[data->buff_m] = *(out-1);
				++data->buff_m;
			}
		}

	}
	return 0;
}

Window::Window(QWidget *parent)
	: QWidget(parent)
{
	QLabel *lab_title = new QLabel("Korg Monotron", this);

	this->sldr_volume = new QSlider(Qt::Horizontal);
	this->sldr_volume->setValue(50);

	this->dl_pitch = new QDial();
	this->dl_pitch->setMinimum(0);
	this->dl_pitch->setMaximum(10000);
	this->dl_pitch->setSingleStep(10);
	this->dl_pitch->setPageStep(100);

	this->dl_harmonics = new QDial();
	this->dl_harmonics->setMinimum(0);
	this->dl_harmonics->setMaximum(100);
	
	this->dl_fc = new QDial();
	this->dl_fa = new QDial();

	this->btn_sound = new QPushButton("Sound!");
	this->btn_disp = new QPushButton("Display!");

	this->dl_refresh = new QDial();
	this->dl_refresh->setMinimum(0);
	this->dl_refresh->setMaximum(500);

	this->dl_trigger = new QDial();
	this->dl_trigger->setMinimum(-200);
	this->dl_trigger->setMaximum(200);

	this->dl_buff = new QDial();
	QHBoxLayout *data_controls = new QHBoxLayout();
	data_controls->addWidget(dl_refresh);
	data_controls->addWidget(dl_trigger);
	data_controls->addWidget(dl_buff);

	QHBoxLayout *lyt_controls = new QHBoxLayout();

	lyt_controls->addWidget(dl_pitch);
	lyt_controls->addWidget(dl_fc);
	lyt_controls->addWidget(dl_fa);

	QVBoxLayout *lyt_main = new QVBoxLayout();

	lyt_main->addWidget(lab_title);
	lyt_main->addWidget(sldr_volume);
	lyt_main->addLayout(lyt_controls);
	lyt_main->addWidget(dl_harmonics);
	lyt_main->addWidget(btn_sound);
	lyt_main->addWidget(btn_disp);
	lyt_main->addLayout(data_controls);

	this->setLayout(lyt_main);
	this->setWindowTitle("Monotron");

	this->connect(sldr_volume, &QSlider::valueChanged, this, &Window::setVolume);

	this->connect(dl_pitch, &QDial::valueChanged, this, &Window::setPitch);
	this->connect(dl_fc, &QDial::valueChanged, this, &Window::setFC);
	this->connect(dl_fa, &QDial::valueChanged, this, &Window::setFA);
	this->connect(dl_harmonics, &QDial::valueChanged, this, &Window::setHarmonics);

	this->connect(btn_sound, &QPushButton::pressed, this, &Window::soundOn);
	this->connect(btn_sound, &QPushButton::released, this, &Window::soundOff);
	this->connect(btn_disp, &QPushButton::pressed, this, &Window::displayData);

	this->connect(dl_refresh, &QDial::valueChanged, this, &Window::setRefresh);
	this->connect(dl_trigger, &QDial::valueChanged, this, &Window::setTrigger);
	this->connect(dl_buff, &QDial::valueChanged, this, &Window::setSamples);

	PaError err;
	err = Pa_Initialize();
	if (err != paNoError) {
		std::cerr << "Error initializing PortAudio:" << std::endl;
		std::cerr << Pa_GetErrorText(err) << std::endl;
		exit(EXIT_FAILURE);
	}


	this->sample_rate = 192000;
	this->data.left_phase = 0.;
	this->data.right_phase = 0.;
	this->data.volume = .5;
	this->data.sound_on = 0;
	this->data.sr = this->sample_rate;
	this->data.pitch = 440;
	this->data.count = 0;
	this->data.harmonics = 0;
	this->data.buff_n = 4096;
	this->data.buff_m = 0;
	this->data.collect = false;

	this->dl_buff->setMinimum(0);
	this->dl_buff->setMaximum(this->data.buff_n);

	err = Pa_OpenDefaultStream(
			&this->stream,
			0,				// no inputs
			2,				// stereo output
			paFloat32,			// 32 bit floating point out
			this->sample_rate,
			paFramesPerBufferUnspecified,	// frames per buffer
			patestCallback,			// callback fn
			&this->data
			);
	if (err != paNoError) {
		std::cerr << "error opening stream" << std::endl;
		std::cerr << Pa_GetErrorText(err) << std::endl;
		exit(EXIT_FAILURE);
	}

	err = Pa_StartStream(this->stream);
	if (err != paNoError) {
		std::cerr << "error starting stream" << std::endl;
		std::cerr << Pa_GetErrorText(err) << std::endl;
		exit(EXIT_FAILURE);
	}

	QLineSeries *series = new QLineSeries();
	series->append(0, 6);
	series->append(2, 4);
	series->append(3, 8);
	series->append(7, 4);
	series->append(10, 5);
	*series << QPointF(11, 1) << QPointF(13, 3) << QPointF(17, 6) << QPointF(18, 3) << QPointF(20, 2);
	
	chart = new QChart();
	chart->legend()->hide();
	chart->addSeries(series);
	chart->createDefaultAxes();
	chart->setTitle("Simple line chart");

	chart_view = new QChartView(this->chart);
	chart_view->setRenderHint(QPainter::Antialiasing);

	lyt_main->addWidget(chart_view);

	refresh = 500;
	trigger = 0;
	nosamples = data.buff_n;

	tmr_refresh = new QTimer(this);
	this->connect(tmr_refresh, SIGNAL(timeout()), this, SLOT(displayData()));
	tmr_refresh->start(refresh);
}

void Window::setRefresh(int value)
{
	refresh = value;
	tmr_refresh->setInterval(value);
}

void Window::setTrigger(int value)
{
	trigger = value/100.;
	this->displayData();
}

void Window::setSamples(int value)
{
	nosamples = value;
	this->displayData();
}

void Window::displayData()
{
	this->data.collect = true;
	while (this->data.collect) {
		usleep(10000); // 10 ms
	}
	QLineSeries *series = new QLineSeries();

	size_t start = 0;
	bool below = false;
    for (size_t i = 1; i < this->data.buff_n && i < static_cast<size_t>(nosamples); ++i) {
		if (this->data.buff[i] > this->data.buff[i-1]) {
			if (this->data.buff[i] <= trigger) {
				below = true;
			}
			if (below && this->data.buff[i] >= trigger) {
				start = i;
				break;
			}
		}
	}
	for (size_t i = start; i < data.buff_n && i < nosamples+start; ++i) {
		double t = (i-start)/(double)data.sr * 1e3;
		QPointF p(t, this->data.buff[i]);
		*series << p;
	}
	chart->removeAllSeries();
	chart->addSeries(series);	
	chart->createDefaultAxes();
}

Window::~Window()
{
	PaError err;
	err = Pa_Terminate();
	if (err != paNoError) {
		std::cerr << "Error terminating PortAudio:" << std::endl;
		std::cerr << Pa_GetErrorText(err) << std::endl;
	} else {
		std::cout << "terminated PortAudio" << std::endl;
	}
}
void Window::setVolume(int value)
{
	this->data.volume = value / 100.;
	std::cout << "setVolume: " << value << std::endl;
}

void Window::setPitch(int value)
{
	double value_scaled = value / static_cast<double>(this->dl_pitch->maximum());
	this->data.pitch = pow(value_scaled*100, 2.15);
	std::cout << "setPitch: " << value << " " << this->data.pitch << std::endl;
}

void Window::setFC(int value)
{
	this->fc_val = value;
	this->data.harmonics = value;

	std::cout << "setFC: " << value << ": " << this->data.harmonics << " harmonics" << std::endl;
}

void Window::setFA(int value)
{
	this->fa_val = value;
	std::cout << "setFA: " << value << std::endl;
}

void Window::setHarmonics(int value)
{
	this->data.harmonics = value;

	std::cout << "setHarmonics: " << value << ": " << this->data.harmonics << " harmonics" << std::endl;
}

void Window::soundOn()
{
	std::cout << "soundOn" << std::endl;
	this->data.sound_on = !this->data.sound_on;
}

void Window::soundOff()
{
	std::cout << "soundOff" << std::endl;
	// this->data.sound_on = 0;
}
