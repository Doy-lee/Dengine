#if 1
#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#include "Dengine/AssetManager.h"
#include "Dengine/Common.h"
#include "Dengine/Debug.h"
#include "Dengine/Math.h"
#include "Dengine/OpenGL.h"

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
	 * INITIALISE AUDIO
	 *******************
	 */
	alGetError();
	// TODO(doyle): Read this http://www.gamedev.net/page/resources/_/technical/game-programming/basic-openal-sound-manager-for-your-project-r3791
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

#define NUM_BUFFERS 3
#define BUFFER_SIZE 4096
	ALuint audioBufferIds[NUM_BUFFERS];
	alGenBuffers(NUM_BUFFERS, audioBufferIds);
	AL_CHECK_ERROR();

	ALuint audioSourcesIds[NUM_BUFFERS];
	alGenBuffers(NUM_BUFFERS, audioSourcesIds);
	AL_CHECK_ERROR();

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

	/*
	 *******************
	 * GAME LOOP
	 *******************
	 */
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
