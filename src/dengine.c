#if 1
#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#define _CRT_SECURE_NO_WARNINGS
#include <STB/stb_vorbis.c>

#include "Dengine/AssetManager.h"
#include "Dengine/Common.h"
#include "Dengine/Debug.h"
#include "Dengine/Math.h"
#include "Dengine/OpenGL.h"
#include "Dengine/Platform.h"

#include "WorldTraveller/WorldTraveller.h"

void alCheckError_(const char *file, int line)
{

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
#define AL_CHECK_ERROR() alCheckError_(__FILE__, __LINE__);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	GameState *game = CAST(GameState *)(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < NUM_KEYS)
	{
		if (action == GLFW_PRESS)
			game->keys[key] = TRUE;
		else if (action == GLFW_RELEASE)
			game->keys[key] = FALSE;
	}
}

void mouse_callback(GLFWwindow *window, double xPos, double yPos) {}

void scroll_callback(GLFWwindow *window, double xOffset, double yOffset) {}

int main()
{
	/*
	 **************************
	 * INIT APPLICATION WINDOW
	 **************************
	 */
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	i32 windowWidth = 1600;
	i32 windowHeight = 900;

	GLFWwindow *window =
	    glfwCreateWindow(windowWidth, windowHeight, "Dengine", NULL, NULL);

	if (!window)
	{
		printf("glfwCreateWindow() failed: Failed to create window\n");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	/*
	 **************************
	 * INITIALISE OPENGL STATE
	 **************************
	 */
	/* Make GLEW use more modern technies for OGL on core profile*/
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		printf("glewInit() failed: Failed to initialise GLEW\n");
		return -1;
	}
	// NOTE(doyle): glewInit() bug that sets the gl error flag after init
	// regardless of success. Catch it once by calling glGetError
	glGetError();

	i32 frameBufferWidth, frameBufferHeight;
	glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
	glViewport(0, 0, frameBufferWidth, frameBufferHeight);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);

	/*
	 *******************
	 * INITIALISE GAME
	 *******************
	 */
	GameState worldTraveller = {0};
	worldTraveller_gameInit(&worldTraveller,
	                        V2i(frameBufferWidth, frameBufferHeight));
#ifdef DENGINE_DEBUG
	debug_init(&worldTraveller.arena, V2i(windowWidth, windowHeight),
	           worldTraveller.assetManager.font);
#endif

	glfwSetWindowUserPointer(window, CAST(void *)(&worldTraveller));

	/*
	 *******************
	 * INITIALISE AUDIO
	 *******************
	 */
	alGetError();
	// TODO(doyle): Read this
	// http://www.gamedev.net/page/resources/_/technical/game-programming/basic-openal-sound-manager-for-your-project-r3791
	// https://gist.github.com/Oddity007/965399
	// https://jogamp.org/joal-demos/www/devmaster/lesson8.html
	// http://basic-converter.proboards.com/thread/818/play-files-using-vorbis-openal
	ALCdevice *deviceAL = alcOpenDevice(NULL);
	if (!deviceAL)
	{
		printf("alcOpenDevice() failed: Failed to init OpenAL device.\n");
		return;
	}

	ALCcontext *contextAL = alcCreateContext(deviceAL, NULL);
	alcMakeContextCurrent(contextAL);
	if (!contextAL)
	{
		printf("alcCreateContext() failed: Failed create AL context.\n");
		return;
	}
	AL_CHECK_ERROR();

	/* Open audio file */
	PlatformFileRead fileRead = {0};
#if 0
	platform_readFileToBuffer(&worldTraveller.arena,
	                          "data/audio/Yuki Kajiura - Swordland.ogg",
	                          &fileRead);
#else
	platform_readFileToBuffer(&worldTraveller.arena,
	                          "data/audio/Nobuo Uematsu - Battle 1.ogg",
	                          &fileRead);
#endif

	i32 error;
	stb_vorbis *vorbisFile = stb_vorbis_open_memory(fileRead.buffer, fileRead.size,
	                                                &error, NULL);
	stb_vorbis_info vorbisInfo = stb_vorbis_get_info(vorbisFile);

	//platform_closeFileRead(&worldTraveller.arena, &fileRead);

	/* Generate number of concurrent audio file listeners */
	ALuint audioSourceId;
	alGenSources(1, &audioSourceId);
	AL_CHECK_ERROR();

	/* Generate audio data buffers */
	ALuint audioBufferId[4];
	alGenBuffers(ARRAY_COUNT(audioBufferId), audioBufferId);
	AL_CHECK_ERROR();


#if 0
	ALuint audioFormat = AL_FORMAT_MONO16;
	if (vorbisInfo.channels == 2) audioFormat = AL_FORMAT_STEREO16;
	i32 audioState;
	alGetSourcei(audioSourceIds[0], AL_SOURCE_STATE, &audioState);
#endif

	/*
	 *******************
	 * GAME LOOP
	 *******************
	 */
	f32 startTime      = CAST(f32)(glfwGetTime());
	f32 secondsElapsed = 0.0f; // Time between current frame and last frame

#if 0
	// TODO(doyle): Get actual monitor refresh rate
	i32 monitorRefreshHz      = 60;
	f32 targetSecondsPerFrame = 1.0f / CAST(f32)(monitorRefreshHz);
#else
	// TODO(doyle): http://gafferongames.com/game-physics/fix-your-timestep/
	// NOTE(doyle): Prevent glfwSwapBuffer until a vertical retrace has
	// occurred, i.e. limit framerate to monitor refresh rate
	glfwSwapInterval(1);
#endif

	while (!glfwWindowShouldClose(window))
	{
		/* Check and call events */
		glfwPollEvents();

		/* Rendering commands here*/
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		worldTraveller_gameUpdateAndRender(&worldTraveller, secondsElapsed);
		GL_CHECK_ERROR();

		/* Swap the buffers */
		glfwSwapBuffers(window);

		f32 endTime    = CAST(f32)glfwGetTime();
		secondsElapsed = endTime - startTime;

#define AUDIO_CHUNK_SIZE 65536
		ALint audioState;
		alGetSourcei(audioSourceId, AL_SOURCE_STATE, &audioState);
		if (audioState == AL_STOPPED || audioState == AL_INITIAL)
		{
			// TODO(doyle): This fixes clicking when reusing old buffers
			if (audioState == AL_STOPPED)
			{
				alDeleteBuffers(ARRAY_COUNT(audioBufferId), audioBufferId);
				alGenBuffers(ARRAY_COUNT(audioBufferId), audioBufferId);
			}

			stb_vorbis_seek_start(vorbisFile);
			for (i32 i = 0; i < ARRAY_COUNT(audioBufferId); i++)
			{
				i16 audioChunk[AUDIO_CHUNK_SIZE] = {0};
				stb_vorbis_get_samples_short_interleaved(
				    vorbisFile, vorbisInfo.channels, audioChunk,
				    AUDIO_CHUNK_SIZE);

				alBufferData(audioBufferId[i], AL_FORMAT_STEREO16, audioChunk,
				             AUDIO_CHUNK_SIZE * sizeof(i16),
				             vorbisInfo.sample_rate);
			}

			alSourceQueueBuffers(audioSourceId, ARRAY_COUNT(audioBufferId),
			                     audioBufferId);
			alSourcePlay(audioSourceId);
		}
		else if (audioState == AL_PLAYING)
		{
			ALint numProcessedBuffers;
			alGetSourcei(audioSourceId, AL_BUFFERS_PROCESSED,
			             &numProcessedBuffers);
			if (numProcessedBuffers > 0)
			{
				ALint numBuffersToUnqueue = 1;
				ALuint emptyBufferId;
				alSourceUnqueueBuffers(audioSourceId, numBuffersToUnqueue,
				                       &emptyBufferId);

				i16 audioChunk[AUDIO_CHUNK_SIZE] = {0};
				i32 sampleCount = stb_vorbis_get_samples_short_interleaved(
				    vorbisFile, vorbisInfo.channels, audioChunk,
				    AUDIO_CHUNK_SIZE);

				/* There are still samples to play */
				if (sampleCount > 0)
				{
					DEBUG_LOG("Buffering new audio data");
					alBufferData(emptyBufferId, AL_FORMAT_STEREO16, audioChunk,
					             sampleCount * vorbisInfo.channels *
					                 sizeof(i16),
					             vorbisInfo.sample_rate);
					alSourceQueueBuffers(audioSourceId, 1, &emptyBufferId);
				}
			}
		}
#if 0
		// TODO(doyle): Busy waiting, should sleep
		while (secondsElapsed < targetSecondsPerFrame)
		{
			endTime        = CAST(f32)(glfwGetTime());
			secondsElapsed = endTime - startTime;
		}
#endif

		LOCAL_PERSIST f32 titleUpdateFrequencyInSeconds = 0.5f;

		titleUpdateFrequencyInSeconds -= secondsElapsed;
		if (titleUpdateFrequencyInSeconds <= 0)
		{
			f32 msPerFrame      = secondsElapsed * 1000.0f;
			f32 framesPerSecond = 1.0f / secondsElapsed;

			char textBuffer[256];
			snprintf(textBuffer, ARRAY_COUNT(textBuffer),
			         "Dengine | %f ms/f | %f fps", msPerFrame, framesPerSecond);

			glfwSetWindowTitle(window, textBuffer);
			titleUpdateFrequencyInSeconds = 0.5f;
		}

		startTime = endTime;
	}

	glfwTerminate();
	return 0;
}

#else
#include <Tutorial.cpp>
#endif
