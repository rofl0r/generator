/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/
 *
 * this file (c) 2022 rofl0r. */

#include <string.h>
#include <stdlib.h>
#include <SDL/SDL.h>

#include "generator.h"
#include "gensound.h"
#include "gensoundp.h"
#include "ui.h"

static struct pcm {
	void *buf;
	size_t size;
	size_t pos;
	unsigned format;
	SDL_mutex *lock;
	SDL_cond *available;
} pcm;

static void audio_callback(void *blah, uint8 *stream, int len)
{
	SDL_LockMutex(pcm.lock);
	if(pcm.pos < len) {
		// on underrun, write as many bytes as we got queued, this
		// should reduce audible artifacts to a minimum
		len = pcm.pos;
	}
	memcpy(stream, pcm.buf, len);
	pcm.pos -= len;
	memmove(pcm.buf, pcm.buf+len, pcm.pos);
	SDL_CondSignal(pcm.available);
	SDL_UnlockMutex(pcm.lock);
}


/*** soundp_start - start sound hardware ***/

int soundp_start(void)
{
	if(pcm.buf) {
		SDL_PauseAudio(0);
		return 0; // already inited
	}
	int i;
	SDL_AudioSpec as = {0}, ob;
#ifdef WORDS_BIGENDIAN
	pcm.format = AUDIO_S16MSB;
#else
	pcm.format = AUDIO_S16LSB;
#endif
	int samplerate = sound_speed;
	SDL_InitSubSystem(SDL_INIT_AUDIO);
	as.freq = samplerate;
	as.format = pcm.format;
        as.channels = 2;
        as.samples = samplerate / 60;
        for (i = 1; i < as.samples; i<<=1);
        as.samples = i;
        as.callback = audio_callback;
        as.userdata = 0;

	pcm.lock = SDL_CreateMutex();
	pcm.available = SDL_CreateCond();

        if (SDL_OpenAudio(&as, &ob) == -1) {
		LOG_CRITICAL(("SDL_OpenAudio failed: %s", SDL_GetError()));
                return 1;
        }

	pcm.size = ob.size * 2;
	pcm.buf = calloc(1, pcm.size);
	pcm.pos = 0;

	SDL_PauseAudio(0);
	return 0;
}

/*** soundp_stop - stop sound hardware ***/

void soundp_stop(void)
{
	SDL_PauseAudio(1);
}

/*** sound_samplesbuffered - how many samples are currently buffered? ***/

int soundp_samplesbuffered(void)
{
	return pcm.pos / 4;
}

static void upload_buffer(void* buffer, size_t n) {
	SDL_LockMutex(pcm.lock);
	while(pcm.pos + n > pcm.size)
		SDL_CondWait(pcm.available, pcm.lock);
	memcpy(pcm.buf+pcm.pos, buffer, n);
	pcm.pos += n;
	SDL_UnlockMutex(pcm.lock);
}

int soundp_output(uint16 *left, uint16 *right, unsigned int samples)
{
	uint16 buffer[(SOUND_MAXRATE / 50) * 2];      /* pal is slowest framerate */
	unsigned int i, o;

	if (samples > (sizeof(buffer) << 1)) {
		LOG_CRITICAL(("Too many samples passed to soundp_output!"));
		return -1;
	}
	for (i = o = 0; i < samples; i++) {
		buffer[o++] = left[i];
		buffer[o++] = right[i];
	}

	upload_buffer(buffer, samples * 4);
	return 0;
}
