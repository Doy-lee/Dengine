#ifndef DENGINE_AUDIO_H
#define DENGINE_AUDIO_H

#include <OpenAL/al.h>

#include "Dengine/Common.h"

struct AudioRenderer
{
	ALuint sourceId[1];
	ALuint bufferId[4];

	AudioVorbis *audio;
	ALuint format;
};

typedef struct AudioRenderer AudioRenderer;

const i32 audio_init();
const i32 audio_rendererInit(AudioRenderer *audioRenderer);
void audio_streamVorbis(AudioRenderer *audioRenderer, AudioVorbis *vorbis);
void audio_updateAndPlay(AudioRenderer *audioRenderer);
#endif
