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

const i32 audio_init()
{
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

	return 0;
}

const i32 audio_rendererInit(AudioRenderer *audioRenderer)
{
	/* Generate number of concurrent audio file listeners */
	alGenSources(ARRAY_COUNT(audioRenderer->sourceId), audioRenderer->sourceId);
	AL_CHECK_ERROR();

	/* Generate audio data buffers */
	alGenBuffers(ARRAY_COUNT(audioRenderer->bufferId), audioRenderer->bufferId);
	AL_CHECK_ERROR();

	return 0;
}

#define AUDIO_CHUNK_SIZE_ 65536
void audio_streamVorbis(AudioRenderer *audioRenderer, AudioVorbis *vorbis)
{
	/* Determine format */
	audioRenderer->format = AL_FORMAT_MONO16;
	if (vorbis->info.channels == 2)
		audioRenderer->format = AL_FORMAT_STEREO16;
	else if (vorbis->info.channels != 1)
		DEBUG_LOG("audio_streamVorbis() warning: Unaccounted channel format");

	audioRenderer->audio = vorbis;
	AudioVorbis *audio   = audioRenderer->audio;
}

void audio_updateAndPlay(AudioRenderer *audioRenderer)
{
	AudioVorbis *audio = audioRenderer->audio;
	if (!audio)
	{
		DEBUG_LOG("audio_updateAndPlay() early exit: No audio stream connected");
		return;
	}

	ALint audioState;
	alGetSourcei(audioRenderer->sourceId[0], AL_SOURCE_STATE, &audioState);
	if (audioState == AL_STOPPED || audioState == AL_INITIAL)
	{
		// TODO(doyle): This fixes clicking when reusing old buffers
		if (audioState == AL_STOPPED)
		{
			alDeleteBuffers(ARRAY_COUNT(audioRenderer->bufferId),
			                audioRenderer->bufferId);
			alGenBuffers(ARRAY_COUNT(audioRenderer->bufferId),
			             audioRenderer->bufferId);
		}

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
		}

		alSourceQueueBuffers(audioRenderer->sourceId[0],
		                     ARRAY_COUNT(audioRenderer->bufferId),
		                     audioRenderer->bufferId);
		alSourcePlay(audioRenderer->sourceId[0]);
	}
	else if (audioState == AL_PLAYING)
	{
		ALint numProcessedBuffers;
		alGetSourcei(audioRenderer->sourceId[0], AL_BUFFERS_PROCESSED,
		             &numProcessedBuffers);
		if (numProcessedBuffers > 0)
		{
			ALint numBuffersToUnqueue = 1;
			ALuint emptyBufferId;
			alSourceUnqueueBuffers(audioRenderer->sourceId[0],
			                       numBuffersToUnqueue, &emptyBufferId);

			i16 audioChunk[AUDIO_CHUNK_SIZE_] = {0};
			i32 sampleCount = stb_vorbis_get_samples_short_interleaved(
			    audio->file, audio->info.channels, audioChunk,
			    AUDIO_CHUNK_SIZE_);

			/* There are still samples to play */
			if (sampleCount > 0)
			{
				DEBUG_LOG("Buffering new audio data");
				alBufferData(emptyBufferId, audioRenderer->format, audioChunk,
				             sampleCount * audio->info.channels * sizeof(i16),
				             audio->info.sample_rate);
				alSourceQueueBuffers(audioRenderer->sourceId[0], 1,
				                     &emptyBufferId);
			}
		}
	}
}
