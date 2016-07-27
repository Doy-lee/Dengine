#if 1
#include "Dengine/AssetManager.h"
#include "Dengine/Audio.h"
#include "Dengine/Common.h"
#include "Dengine/Debug.h"
#include "Dengine/Math.h"
#include "Dengine/OpenGL.h"
#include "Dengine/Platform.h"

#include "WorldTraveller/WorldTraveller.h"

// TODO(doyle): Temporary
struct AudioRenderer;

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
	i32 result = audio_init();
	if (result)
	{
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
	}

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
