#ifndef DENGINE_AUDIO_H
#define DENGINE_AUDIO_H

#include <OpenAL/al.h>

#include "Dengine/Common.h"

#define AUDIO_NO_FREE_SOURCE -1
typedef struct AudioSourceEntry
{
	u32 id;
	i32 nextFreeIndex;
} AudioSourceEntry;

#define AUDIO_MAX_SOURCES 32
typedef struct AudioManager
{
	// NOTE(doyle): Source entries point to the next free index
	AudioSourceEntry sourceList[AUDIO_MAX_SOURCES];
	i32 freeSourceIndex;

} AudioManager;


#define AUDIO_REPEAT_INFINITE -10
#define AUDIO_SOURCE_UNASSIGNED -1
typedef struct AudioRenderer
{
	i32 sourceIndex;
	ALuint bufferId[4];

	AudioVorbis *audio;
	ALuint format;
	i32 numPlays;
} AudioRenderer;


const i32 audio_init(AudioManager *audioManager);
const i32 audio_streamPlayVorbis(AudioManager *audioManager,
                                 AudioRenderer *audioRenderer,
                                 AudioVorbis *vorbis, i32 numPlays);
const i32 audio_streamStopVorbis(AudioManager *audioManager,
                                 AudioRenderer *audioRenderer);
const i32 audio_streamPauseVorbis(AudioManager *audioManager,
                                  AudioRenderer *audioRenderer);
const i32 audio_streamResumeVorbis(AudioManager *audioManager,
                                   AudioRenderer *audioRenderer);
const i32 audio_updateAndPlay(AudioManager *audioManager,
                              AudioRenderer *audioRenderer);
#endif