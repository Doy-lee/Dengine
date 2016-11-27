#ifndef DENGINE_AUDIO_H
#define DENGINE_AUDIO_H

#include <OpenAL/al.h>

#include "Dengine/Common.h"

/* Forward Declaration */
typedef struct MemoryArena MemoryArena_;

#define AUDIO_NO_FREE_SOURCE -1
typedef struct AudioSourceEntry
{
	u32 id;
	b32 isFree;
} AudioSourceEntry;

// TODO(doyle): Incorporate memory arena into managers?
#define AUDIO_MAX_SOURCES 32
typedef struct AudioManager
{
	// NOTE(doyle): Source entries point to the next free index
	AudioSourceEntry sourceList[AUDIO_MAX_SOURCES];
} AudioManager;

enum AudioState
{
	audiostate_stopped = 0,
	audiostate_paused,
	audiostate_playing,
	audiostate_count,
};

// TODO(doyle): Add object pool for assets, in particular audio
#define AUDIO_REPEAT_INFINITE -10
#define AUDIO_SOURCE_UNASSIGNED -1
typedef struct AudioRenderer
{
	i32 sourceIndex;
	ALuint bufferId[3];

	AudioVorbis *audio;
	ALuint format;
	i32 numPlays;

	b32 isStreaming;
	enum AudioState state;
} AudioRenderer;


const i32 audio_init(AudioManager *audioManager);
const i32 audio_vorbisPlay(MemoryArena_ *arena, AudioManager *audioManager,
                           AudioRenderer *audioRenderer, AudioVorbis *vorbis,
                           i32 numPlays);
const i32 audio_vorbisStream(MemoryArena_ *arena, AudioManager *audioManager,
                                 AudioRenderer *audioRenderer,
                                 AudioVorbis *vorbis, i32 numPlays);
const i32 audio_vorbisStop(MemoryArena_ *arena, AudioManager *audioManager,
                                 AudioRenderer *audioRenderer);
const i32 audio_vorbisPause(AudioManager *audioManager,
                                  AudioRenderer *audioRenderer);
const i32 audio_vorbisResume(AudioManager *audioManager,
                                   AudioRenderer *audioRenderer);
const i32 audio_updateAndPlay(MemoryArena_ *arena, AudioManager *audioManager,
                              AudioRenderer *audioRenderer);
#endif
