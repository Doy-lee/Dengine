#include <OpenAL/alc.h>

#define STB_VORBIS_HEADER_ONLY
#include <STB/stb_vorbis.c>

#include "Dengine/Assets.h"
#include "Dengine/Audio.h"
#include "Dengine/Debug.h"
#include "Dengine/MemoryArena.h"
#include "dengine/Platform.h"

#define AL_CHECK_ERROR() alCheckError_(__FILE__, __LINE__);
void alCheckError_(const char *file, int line)
{
	// NOTE(doyle): OpenAL error stack is 1 deep
	ALenum errorCode;
	while ((errorCode = alGetError()) != AL_NO_ERROR)
	{
		printf("OPENAL ");
		switch(errorCode)
		{
		case AL_INVALID_NAME:
			printf("INVALID_NAME | ");
			break;
		case AL_INVALID_ENUM:
			printf("INVALID_ENUM | ");
			break;
		case AL_INVALID_VALUE:
			printf("INVALID_VALUE | ");
			break;
		case AL_INVALID_OPERATION:
			printf("INVALID_OPERATION | ");
			break;
		case AL_OUT_OF_MEMORY:
			printf("OUT_OF_MEMORY | ");
			break;
		default:
			printf("UNRECOGNISED ERROR CODE | ");
			break;
		}
		printf("Error %08x, %s (%d)\n", errorCode, file, line);
	}
};

const i32 audio_init(AudioManager *audioManager)
{
#ifdef DENGINE_DEBUG
	ASSERT(audioManager);
#endif
	/* Clear error stack */
	alGetError();

	ALboolean enumerateAudioDevice =
	    alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");

	const ALCchar *device = NULL;
	if (enumerateAudioDevice == AL_TRUE)
	{
		// TODO(doyle): Actually allow users to choose device to output
		/*
		  The OpenAL specification says that the list of devices is organized as
		  a string devices are separated with a NULL character and the list is
		  terminated by two NULL characters.

		  alcGetString with NULL = get all device spcifiers not a particular one
		*/
		device        = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
		const ALCchar *next = device + 1;
		size_t len = 0;

		printf("Devices list:\n");
		printf("----------\n");
		while (device && *device != '\0' && next && *next != '\0')
		{
			printf("%s\n", device);
			len = common_strlen(device);
			device += (len + 1);
			next += (len + 2);
		}
		printf("----------\n");
	}

	/* Get audio device */
	ALCdevice *deviceAL = alcOpenDevice(device);
	if (!deviceAL)
	{
		printf("alcOpenDevice() failed: Failed to init OpenAL device.\n");
		return -1;
	}

	/* Set device context */
	ALCcontext *contextAL = alcCreateContext(deviceAL, NULL);
	alcMakeContextCurrent(contextAL);
	if (!contextAL)
	{
		printf("alcCreateContext() failed: Failed create AL context.\n");
		return -1;
	}
	AL_CHECK_ERROR();

	for (i32 i = 0; i < ARRAY_COUNT(audioManager->sourceList); i++)
	{
		alGenSources(1, &audioManager->sourceList[i].id);
		AL_CHECK_ERROR();

		audioManager->sourceList[i].isFree = TRUE;
	}

	return 0;
}

INTERNAL inline u32 getSourceId(AudioManager *audioManager,
                                AudioRenderer *audioRenderer)
{
	u32 result = audioManager->sourceList[audioRenderer->sourceIndex].id;
	return result;
}

INTERNAL i32 rendererAcquire(MemoryArena *arena, AudioManager *audioManager,
                             AudioRenderer *audioRenderer)
{
#ifdef DENGINE_DEBUG
	ASSERT(arena && audioManager && audioRenderer);
#endif
	u32 checkSource = getSourceId(audioManager, audioRenderer);
	if (alIsSource(checkSource) == AL_TRUE)
	{
#if 0
		DEBUG_LOG(
		    "rendererAcquire(): Renderer has not been released before "
		    "acquiring, force release by stopping stream");
#endif
		audio_stopVorbis(arena, audioManager, audioRenderer);
	}

	// TODO(doyle): Super bad linear O(n) search for every audio-enabled entity
	i32 vacantSource = AUDIO_SOURCE_UNASSIGNED;
	for (i32 i = 0; i < ARRAY_COUNT(audioManager->sourceList); i++)
	{
		if (audioManager->sourceList[i].isFree)
		{
			vacantSource = i;
			audioManager->sourceList[i].isFree = FALSE;
			break;
		}
	}

	if (vacantSource == AUDIO_SOURCE_UNASSIGNED)
	{
		DEBUG_LOG("rendererAcquire(): Failed to acquire free source, all busy");
		return -1;
	}

	audioRenderer->sourceIndex = vacantSource;

	/* Generate audio data buffers */
	alGenBuffers(ARRAY_COUNT(audioRenderer->bufferId), audioRenderer->bufferId);
	AL_CHECK_ERROR();

	//alSourcef(audioRenderer->sourceId[0], AL_PITCH, 2.0f);

	return 0;
}

INTERNAL const i32 rendererRelease(MemoryArena *arena, AudioManager *audioManager,
                                   AudioRenderer *audioRenderer)
{

	i32 result = 0;
	u32 alSourceId = getSourceId(audioManager, audioRenderer);
	if (alIsSource(alSourceId) == AL_FALSE)
	{
		DEBUG_LOG(
		    "rendererRelease(): Trying to release invalid source, early exit");

		result = -1;
		return result;
	}

	/*
	  NOTE(doyle): Doing an alSourceRewind will result in the source going to
	  the AL_INITIAL state, but the properties of the source (position,
	  velocity, etc.) will not change.
	*/
	alSourceRewind(alSourceId);
	AL_CHECK_ERROR();

	// TODO(doyle): We can possible remove this by just attaching the null buffer ..
	ALint numProcessedBuffers;
	alGetSourcei(alSourceId, AL_BUFFERS_PROCESSED, &numProcessedBuffers);
	if (numProcessedBuffers > 0)
	{
		alSourceUnqueueBuffers(alSourceId, numProcessedBuffers,
		                       audioRenderer->bufferId);
	}
	AL_CHECK_ERROR();

	// NOTE(doyle): Any buffer queued up that has not been played cannot be
	// deleted without being played once. We can set the source buffers to NULL
	// (0) to free up the buffer, since we still hold the reference ids for the
	// buffer in our audio structure we can delete it afterwards ..
	alSourcei(alSourceId, AL_BUFFER, 0);
	alDeleteBuffers(ARRAY_COUNT(audioRenderer->bufferId),
	                audioRenderer->bufferId);
	AL_CHECK_ERROR();

	for (i32 i = 0; i < ARRAY_COUNT(audioRenderer->bufferId); i++)
	{
		audioRenderer->bufferId[i] = 0;
	}

	if (audioRenderer->isStreaming)
	{
		stb_vorbis_close(audioRenderer->audio->file);
		PLATFORM_MEM_FREE(arena, audioRenderer->audio, sizeof(AudioVorbis));
	}

	audioRenderer->audio    = NULL;
	audioRenderer->numPlays = 0;

	u32 sourceIndexToFree      = audioRenderer->sourceIndex;
	audioRenderer->sourceIndex = AUDIO_SOURCE_UNASSIGNED;

	audioManager->sourceList[sourceIndexToFree].isFree = TRUE;

	return result;
}

INTERNAL i32 initRendererForPlayback(MemoryArena *arena,
                                     AudioManager *audioManager,
                                     AudioRenderer *audioRenderer,
                                     AudioVorbis *vorbis, i32 numPlays)
{
#ifdef DENGINE_DEBUG
	ASSERT(audioManager && audioRenderer && vorbis);
	if (numPlays != AUDIO_REPEAT_INFINITE && numPlays <= 0)
	{
		DEBUG_LOG("audio_streamPlayVorbis() warning: Number of plays is less than 0");
	}
#endif

	i32 result = rendererAcquire(arena, audioManager, audioRenderer);
	if (result)
	{
		DEBUG_LOG("audio_streamPlayVorbis() failed: Could not acquire renderer");
		return result;
	}

	/* Determine format */
	audioRenderer->format = AL_FORMAT_MONO16;
	if (vorbis->info.channels == 2)
		audioRenderer->format = AL_FORMAT_STEREO16;
	else if (vorbis->info.channels != 1)
	{
#ifdef DENGINE_DEBUG
		DEBUG_LOG(
		    "audio_streamPlayVorbis() warning: Unexpected channel format");
#endif
	}

	audioRenderer->numPlays = numPlays;

	return result;
}

const i32 audio_playVorbis(MemoryArena *arena, AudioManager *audioManager,
                           AudioRenderer *audioRenderer, AudioVorbis *vorbis,
                           i32 numPlays)
{
	i32 result = initRendererForPlayback(arena, audioManager, audioRenderer,
	                                     vorbis, numPlays);

	i16 *soundSamples;
	i32 channels, sampleRate;
	i32 numSamples = stb_vorbis_decode_memory(
	    vorbis->data, vorbis->size, &channels, &sampleRate, &soundSamples);
	alBufferData(audioRenderer->bufferId[0], audioRenderer->format,
	             soundSamples, numSamples * vorbis->info.channels * sizeof(i16),
	             vorbis->info.sample_rate);

	audioRenderer->audio       = vorbis;
	audioRenderer->isStreaming = FALSE;

	return result;
}

const i32 audio_streamPlayVorbis(MemoryArena *arena, AudioManager *audioManager,
                                 AudioRenderer *audioRenderer,
                                 AudioVorbis *vorbis, i32 numPlays)
{

	i32 result = initRendererForPlayback(arena, audioManager, audioRenderer,
	                                     vorbis, numPlays);
	// NOTE(doyle): We make a copy of the audio vorbis file using all the same
	// data except the file pointer. If the same sound is playing twice
	// simultaneously, we need unique file pointers into the data to track song
	// position uniquely
	AudioVorbis *copyAudio     = PLATFORM_MEM_ALLOC(arena, 1, AudioVorbis);
	copyAudio->type            = vorbis->type;
	copyAudio->info            = vorbis->info;
	copyAudio->lengthInSamples = vorbis->lengthInSamples;
	copyAudio->lengthInSeconds = vorbis->lengthInSeconds;

	copyAudio->data            = vorbis->data;
	copyAudio->size            = vorbis->size;

	i32 error;
	copyAudio->file =
	    stb_vorbis_open_memory(copyAudio->data, copyAudio->size, &error, NULL);

	audioRenderer->audio       = copyAudio;
	audioRenderer->isStreaming = TRUE;

	return result;
}

const i32 audio_stopVorbis(MemoryArena *arena, AudioManager *audioManager,
                           AudioRenderer *audioRenderer)
{
	i32 result     = 0;
	u32 alSourceId = getSourceId(audioManager, audioRenderer);
	if (alIsSource(alSourceId) == AL_TRUE)
	{
		alSourceStop(alSourceId);
		AL_CHECK_ERROR();
		result = rendererRelease(arena, audioManager, audioRenderer);
	}
	else
	{
#ifdef DENGINE_DEBUG
		if (audioRenderer->audio)
		{
			DEBUG_LOG(
			    "audio_streamStopVorbis(): Tried to stop invalid source, but "
			    "renderer has valid audio ptr");
		}
#endif
		result = -1;
	}

	return result;
}

const i32 audio_pauseVorbis(AudioManager *audioManager,
                            AudioRenderer *audioRenderer)
{
	i32 result     = 0;
	u32 alSourceId = getSourceId(audioManager, audioRenderer);
	if (alIsSource(alSourceId) == AL_TRUE)
	{
		alSourcePause(alSourceId);
		AL_CHECK_ERROR();
	}
	else
	{
		DEBUG_LOG("audio_streamPauseVorbis(): Tried to pause invalid source");
		result = -1;
	}
	return result;
}

const i32 audio_resumeVorbis(AudioManager *audioManager,
                             AudioRenderer *audioRenderer)
{
	i32 result = 0;
	u32 alSourceId = getSourceId(audioManager, audioRenderer);
	if (alIsSource(alSourceId) == AL_TRUE)
	{
		alSourcePlay(alSourceId);
		AL_CHECK_ERROR();
	}
	else
	{
#ifdef DENGINE_DEBUG
		DEBUG_LOG("audio_resumeVorbis(): Tried to resume invalid source")
#endif
		result = -1;
	}

	return result;
}

#define AUDIO_CHUNK_SIZE_ 65536
const i32 audio_updateAndPlay(MemoryArena *arena, AudioManager *audioManager,
                              AudioRenderer *audioRenderer)
{
	AudioVorbis *audio = audioRenderer->audio;
	if (!audio) return 0;

#if 0
	if (audioRenderer->numPlays != AUDIO_REPEAT_INFINITE &&
	    audioRenderer->numPlays <= 0)
	{
		i32 result = rendererRelease(arena, audioManager, audioRenderer);
		return result;
	}
#endif

	u32 alSourceId = getSourceId(audioManager, audioRenderer);
	if (alIsSource(alSourceId) == AL_FALSE)
	{
		DEBUG_LOG("audio_updateAndPlay(): Update failed on invalid source id");
		return -1;
	}

	ALint audioState;
	alGetSourcei(alSourceId, AL_SOURCE_STATE, &audioState);
	AL_CHECK_ERROR();
	if (audioState == AL_STOPPED || audioState == AL_INITIAL)
	{
		if (audioState == AL_STOPPED)
		{
			if (audioRenderer->numPlays != AUDIO_REPEAT_INFINITE)
				audioRenderer->numPlays--;

			if (audioRenderer->numPlays == AUDIO_REPEAT_INFINITE ||
			    audioRenderer->numPlays > 0)
			{
				// TODO(doyle): Delete and recreate fixes clicking when reusing
				// buffers
				alDeleteBuffers(ARRAY_COUNT(audioRenderer->bufferId),
				                audioRenderer->bufferId);
				AL_CHECK_ERROR();

				alGenBuffers(ARRAY_COUNT(audioRenderer->bufferId),
				             audioRenderer->bufferId);
				AL_CHECK_ERROR();
			}
			else
			{
				i32 result =
				    rendererRelease(arena, audioManager, audioRenderer);
				return result;
			}
		}

		if (audioRenderer->isStreaming)
		{
			stb_vorbis_seek_start(audio->file);
			for (i32 i = 0; i < ARRAY_COUNT(audioRenderer->bufferId); i++)
			{
				i16 audioChunk[AUDIO_CHUNK_SIZE_] = {0};
				stb_vorbis_get_samples_short_interleaved(
				    audio->file, audio->info.channels, audioChunk,
				    AUDIO_CHUNK_SIZE_);

				alBufferData(audioRenderer->bufferId[i], audioRenderer->format,
				             audioChunk, AUDIO_CHUNK_SIZE_ * sizeof(i16),
				             audio->info.sample_rate);
				AL_CHECK_ERROR();
			}

			alSourceQueueBuffers(alSourceId,
			                     ARRAY_COUNT(audioRenderer->bufferId),
			                     audioRenderer->bufferId);
		}
		else
		{
			alSourceQueueBuffers(alSourceId, 1, audioRenderer->bufferId);
		}

		AL_CHECK_ERROR();
		alSourcePlay(alSourceId);
		AL_CHECK_ERROR();
	}
	else if (audioState == AL_PLAYING)
	{
		ALint numProcessedBuffers;
		alGetSourcei(alSourceId, AL_BUFFERS_PROCESSED,
		             &numProcessedBuffers);
		AL_CHECK_ERROR();
		if (numProcessedBuffers > 0)
		{
			// TODO(doyle): Possibly wrong, we should pass in all processed buffers?
			ALint numBuffersToUnqueue = 1;
			ALuint emptyBufferId;
			alSourceUnqueueBuffers(alSourceId, numBuffersToUnqueue,
			                       &emptyBufferId);
			AL_CHECK_ERROR();

			i16 audioChunk[AUDIO_CHUNK_SIZE_] = {0};
			i32 sampleCount = stb_vorbis_get_samples_short_interleaved(
			    audio->file, audio->info.channels, audioChunk,
			    AUDIO_CHUNK_SIZE_);

			/* There are still samples to play */
			if (sampleCount > 0)
			{
				alBufferData(emptyBufferId, audioRenderer->format, audioChunk,
				             sampleCount * audio->info.channels * sizeof(i16),
				             audio->info.sample_rate);
				AL_CHECK_ERROR();
				alSourceQueueBuffers(alSourceId, 1, &emptyBufferId);
				AL_CHECK_ERROR();
			}
		}
	}

	return 0;
}
