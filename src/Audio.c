#include <OpenAL/alc.h>

#define STB_VORBIS_HEADER_ONLY
#include <STB/stb_vorbis.c>

#include "Dengine/Assets.h"
#include "Dengine/Audio.h"
#include "Dengine/Debug.h"

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

		// NOTE(doyle): If last entry, loop the free source to front of list
		if (i + 1 >= ARRAY_COUNT(audioManager->sourceList))
			audioManager->sourceList[i].nextFreeIndex = 0;
		else
			audioManager->sourceList[i].nextFreeIndex = i+1;
	}
	audioManager->freeSourceIndex = 0;

	return 0;
}

INTERNAL inline u32 getSourceId(AudioManager *audioManager,
                                AudioRenderer *audioRenderer)
{
	u32 result = audioManager->sourceList[audioRenderer->sourceIndex].id;
	return result;
}

INTERNAL i32 rendererAcquire(AudioManager *audioManager,
                             AudioRenderer *audioRenderer)
{

	i32 vacantSource = audioManager->freeSourceIndex;

#ifdef DENGINE_DEBUG
	ASSERT(audioManager && audioRenderer);
	ASSERT(vacantSource >= 0);
#endif

	if (audioManager->sourceList[vacantSource].nextFreeIndex ==
	    AUDIO_NO_FREE_SOURCE)
	{
		// TODO(doyle): Error messaging return paths
		return -1;
	}

	/* Assign a vacant source slot to renderer */
	audioRenderer->sourceIndex = vacantSource;

	/* Update the immediate free source index */
	audioManager->freeSourceIndex =
	    audioManager->sourceList[vacantSource].nextFreeIndex;

	/* Mark current source as in use */
	audioManager->sourceList[vacantSource].nextFreeIndex = AUDIO_NO_FREE_SOURCE;

	/* Generate audio data buffers */
	alGenBuffers(ARRAY_COUNT(audioRenderer->bufferId), audioRenderer->bufferId);
	AL_CHECK_ERROR();

	//alSourcef(audioRenderer->sourceId[0], AL_PITCH, 2.0f);

	return 0;
}

INTERNAL void rendererRelease(AudioManager *audioManager,
                             AudioRenderer *audioRenderer)
{

	u32 alSourceId = getSourceId(audioManager, audioRenderer);

	alSourceUnqueueBuffers(alSourceId, ARRAY_COUNT(audioRenderer->bufferId),
	                       audioRenderer->bufferId);
	AL_CHECK_ERROR();
	alDeleteBuffers(ARRAY_COUNT(audioRenderer->bufferId),
	                audioRenderer->bufferId);
	AL_CHECK_ERROR();

	for (i32 i = 0; i < ARRAY_COUNT(audioRenderer->bufferId); i++)
	{
		audioRenderer->bufferId[i] = 0;
	}

	audioRenderer->audio       = NULL;
	audioRenderer->numPlays    = 0;

	u32 sourceIndexToFree      = audioRenderer->sourceIndex;
	audioRenderer->sourceIndex = AUDIO_SOURCE_UNASSIGNED;

	audioManager->sourceList[sourceIndexToFree].nextFreeIndex =
	    audioManager->freeSourceIndex;
	audioManager->freeSourceIndex = sourceIndexToFree;

}

#define AUDIO_CHUNK_SIZE_ 65536
void audio_streamPlayVorbis(AudioManager *audioManager,
                             AudioRenderer *audioRenderer, AudioVorbis *vorbis,
                             i32 numPlays)
{
#ifdef DENGINE_DEBUG
	ASSERT(audioManager && audioRenderer && vorbis);
	if (numPlays != AUDIO_REPEAT_INFINITE && numPlays <= 0)
	{
		DEBUG_LOG("audio_streamPlayVorbis() warning: Number of plays is less than 0");
	}
#endif

	i32 result = rendererAcquire(audioManager, audioRenderer);
	if (result)
	{
		DEBUG_LOG("audio_streamPlayVorbis() failed: Could not acquire renderer");
		return;
	}

	/* Determine format */
	audioRenderer->format = AL_FORMAT_MONO16;
	if (vorbis->info.channels == 2)
		audioRenderer->format = AL_FORMAT_STEREO16;
	else if (vorbis->info.channels != 1)
	{
#ifdef DENGINE_DEBUG
		DEBUG_LOG(
		    "audio_streamPlayVorbis() warning: Unaccounted channel format");
		ASSERT(INVALID_CODE_PATH);
#endif
	}

	audioRenderer->audio    = vorbis;
	audioRenderer->numPlays = numPlays;
}

void audio_streamStopVorbis(AudioManager *audioManager,
                           AudioRenderer *audioRenderer)
{
	u32 alSourceId = getSourceId(audioManager, audioRenderer);
	alSourceStop(alSourceId);
	AL_CHECK_ERROR();
	rendererRelease(audioManager, audioRenderer);
}

void audio_updateAndPlay(AudioManager *audioManager,
                         AudioRenderer *audioRenderer)
{
	AudioVorbis *audio = audioRenderer->audio;
	if (!audio) return;

	if (audioRenderer->numPlays != AUDIO_REPEAT_INFINITE &&
	    audioRenderer->numPlays <= 0)
	{
		rendererRelease(audioManager, audioRenderer);
		return;
	}

	u32 alSourceId = getSourceId(audioManager, audioRenderer);
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
				rendererRelease(audioManager, audioRenderer);
				return;
			}
		}

		// TODO(doyle): Possible bug! Multiple sources playing same file seeking
		// file ptr to start may interrupt other stream
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

		alSourceQueueBuffers(alSourceId, ARRAY_COUNT(audioRenderer->bufferId),
		                     audioRenderer->bufferId);
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
}
