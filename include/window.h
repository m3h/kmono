#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QtWidgets>
#include <QtCharts>

#include <stdlib.h>
// PortAudio
#include "portaudio.h"

typedef struct
{
	float left_phase;
	float right_phase;
	float volume;
	int sound_on;
	int sr;
	float pitch;
	size_t count;
	int harmonics;

	float buff[4096];
	size_t buff_n;
	size_t buff_m;

	bool collect;
}   
paTestData;
/* This routine will be called by the PortAudio engine when audio is needed.
 * It may called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
 */ 
//static int patestCallback( const void *inputBuffer, void *outputBuffer,
//		unsigned long framesPerBuffer,
//		const PaStreamCallbackTimeInfo* timeInfo,
//		PaStreamCallbackFlags statusFlags,
//		void *userData );

class Window: public QWidget
{
	Q_OBJECT

	public:
		Window(QWidget *parent = nullptr);
		~Window();

	public slots:
		void setVolume(int value);
		void setPitch(int value);
		void setHarmonics(int value);
		void setFC(int value);
		void setFA(int value);

		void setRefresh(int value);
		void setTrigger(int value);
		void setSamples(int value);

		void soundOn();
		void soundOff();

		void displayData();

	private:
		QSlider *sldr_volume;
		QDial *dl_pitch;
		QDial *dl_fc;
		QDial *dl_fa;
		QDial *dl_harmonics;
		QPushButton *btn_sound;
		QPushButton *btn_disp;

        	QChartView *chart_view;
        	QChart *chart;

		QDial *dl_refresh;
		QDial *dl_trigger;
		QDial *dl_buff;

		QTimer *tmr_refresh;
		int refresh;
		float trigger;
		int nosamples;

		double volume;
		double pitch_val, fc_val, fa_val;
		bool sound;

		// audio callback things?
		PaStream *stream;
		paTestData data;
		int sample_rate;

};

#endif // WINDOW_H
