#include <simulation/Sound.h>
#include <list>
#include <math.h>
#include <pthread.h>

std::list<Sound::Note> queue;
pthread_mutex_t Sound::Mutex;

void Sound::AddNote(int duration, double frequency, double volume)
{
	if(queue.size()<128)
	{
		pthread_mutex_lock(&Sound::Mutex);
		Sound::Note n;
		n.duration = duration;
		n.left = duration+1;
		n.frequency = 2*M_PI*frequency;
		n.volume = volume*10;
		n.offset = 0;
		n.fade = 0;
		queue.push_back(n);
		pthread_mutex_unlock(&Sound::Mutex);
	}
}

void Sound::ClearNotes()
{
	pthread_mutex_lock(&Sound::Mutex);
	queue.clear();
	pthread_mutex_unlock(&Sound::Mutex);
}

void Sound::UpdateNotes()
{
	pthread_mutex_lock(&Sound::Mutex);
	for(std::list<Sound::Note>::iterator iter = queue.begin(), end = queue.end(); iter != end;)
	{
		if(iter->left)
		{
			iter->left--;
			if (iter->left==0)
				iter->fade = 0;
			iter++;
		}
		else
			iter = queue.erase(iter);
	}
	pthread_mutex_unlock(&Sound::Mutex);
}

double sfrac(double x)
{
	return (int)x > x-.5f?-1.f:1.f;
}

void Sound::Callback(void *udata, Uint8 *s, int len)
{
	signed char *stream = (signed char *)s;
	pthread_mutex_lock(&Sound::Mutex);
	for(;len;len--)
	{
		double v=0;
		for(std::list<Sound::Note>::iterator iter = queue.begin(), end = queue.end(); iter != end; iter++)
		{
			// Sine wave, with some harmonics to make it sound more interesting
			// (first few harmonics of a square wave at the moment)
			double tmp = sin(iter->offset*iter->frequency/SAMPLES)*iter->volume
					+ 0.333*sin(3*iter->offset*iter->frequency/SAMPLES)*iter->volume
					+ 0.2*sin(5*iter->offset*iter->frequency/SAMPLES)*iter->volume;
			// Fade in/out smoothly, to reduce "clicking" noises due to discontinuities
			// (biggest problem is end of note, but may as well add a bit of smoothing at the start too)
			if (iter->left==0)
			{
				if (iter->fade<300)
				{
					// raised cosine, function values could be pre-calculated if necessary
					tmp *= 0.5*(cos(M_PI*iter->fade/300)+1);
					iter->fade++;
				}
				else tmp = 0;
			}
			else if (iter->fade<50)
			{
				tmp *= 0.5*(cos(M_PI*(50-iter->fade)/50)+1);
				iter->fade++;
			}
			v += tmp;
			iter->offset++;
		}
		int intV = v;
		if (intV>127)
			*stream = 127;
		else if (intV<-127)
			*stream = -127;
		else
			*stream = intV;
		stream++;
	}
	pthread_mutex_unlock(&Sound::Mutex);
}
