#include "Dengine/AssetManager.h"
#include "Dengine/Common.h"
#include "Dengine/Debug.h"
#include "Dengine/Math.h"
#include "Dengine/OpenGL.h"
#include "Dengine/WorldTraveller.h"

INTERNAL inline void processKey(b32 *currState, int key, int action)
{
	if (action == GLFW_PRESS) *currState = TRUE;
	else if (action == GLFW_RELEASE) *currState = FALSE;
}

INTERNAL void keyCallback(GLFWwindow *window, int key, int scancode, int action,
                          int mode)
{
	GameState *game = CAST(GameState *)(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	switch (key)
	{
	case GLFW_KEY_UP:
		processKey(&game->input.up, key, action);
		break;
	case GLFW_KEY_DOWN:
		processKey(&game->input.down, key, action);
		break;
	case GLFW_KEY_LEFT:
		processKey(&game->input.left, key, action);
		break;
	case GLFW_KEY_RIGHT:
		processKey(&game->input.right, key, action);
		break;
	case GLFW_KEY_SPACE:
		processKey(&game->input.space, key, action);
		break;
	case GLFW_KEY_LEFT_SHIFT:
		processKey(&game->input.leftShift, key, action);
		break;
	default:
		break;
	}
}

INTERNAL void mouseCallback(GLFWwindow *window, double xPos, double yPos)
{
	GameState *game = CAST(GameState *)(glfwGetWindowUserPointer(window));

	// NOTE(doyle): x(0), y(0) of mouse starts from the top left of window
	v2 windowSize      = game->renderer.size;
	f32 flipYPos       = windowSize.h - CAST(f32) yPos;
	game->input.mouseP = V2(CAST(f32) xPos, flipYPos);
}

INTERNAL void mouseButtonCallback(GLFWwindow *window, int button, int action,
                                  int mods)
{
	GameState *game = CAST(GameState *)(glfwGetWindowUserPointer(window));

	switch(button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		processKey(&game->input.mouseLeft, button, action);
		break;
	default:
		break;
	}
}

INTERNAL void scrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
}

i32 main(void)
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

	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetScrollCallback(window, scrollCallback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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
