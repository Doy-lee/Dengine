#include "Dengine/AssetManager.h"
#include "Dengine/Common.h"
#include "Dengine/Debug.h"
#include "Dengine/Math.h"
#include "Dengine/OpenGL.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/Asteroid.h"

INTERNAL inline void processKey(KeyState *state, int action)
{
	if (action == GLFW_PRESS || action == GLFW_RELEASE)
	{
		state->newHalfTransitionCount++;
	}
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
		processKey(&game->input.keys[keycode_up], action);
		break;
	case GLFW_KEY_DOWN:
		processKey(&game->input.keys[keycode_down], action);
		break;
	case GLFW_KEY_LEFT:
		processKey(&game->input.keys[keycode_left], action);
		break;
	case GLFW_KEY_RIGHT:
		processKey(&game->input.keys[keycode_right], action);
		break;
	case GLFW_KEY_LEFT_SHIFT:
		processKey(&game->input.keys[keycode_leftShift], action);
		break;
	case GLFW_KEY_ENTER:
		processKey(&game->input.keys[keycode_enter], action);
		break;
	case GLFW_KEY_BACKSPACE:
		processKey(&game->input.keys[keycode_backspace], action);
		break;
	case GLFW_KEY_TAB:
		processKey(&game->input.keys[keycode_tab], action);
		break;
	default:
		if (key >= ' ' && key <= '~')
		{
			i32 offset = 0;
			if (key >= 'A' && key <= 'Z')
			{
				KeyState *leftShiftKey = &game->input.keys[keycode_leftShift];
				if (!common_isSet(leftShiftKey->flags, keystateflag_ended_down))
					offset = 'a' - 'A';
			}

			i32 glfwCodeToPlatformCode = (key - ' ') + offset;
			processKey(&game->input.keys[glfwCodeToPlatformCode], action);
		}
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
		processKey(&game->input.keys[keycode_mouseLeft], action);
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
	v2 windowSize = V2i(frameBufferWidth, frameBufferHeight);

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
	Memory memory = {0};
	MemoryIndex persistentSize = MEGABYTES(32);
	MemoryIndex transientSize  = MEGABYTES(32);

	memory.persistentSize = persistentSize;
	memory.persistent     = PLATFORM_MEM_ALLOC_(NULL, persistentSize, u8);

	memory.transientSize = transientSize;
	memory.transient     = PLATFORM_MEM_ALLOC_(NULL, transientSize, u8);

	MemoryArena_ gameArena = {0};
	memory_arenaInit(&gameArena, memory.persistent, memory.persistentSize);

	GameState *gameState = MEMORY_PUSH_STRUCT(&gameArena, GameState);
	gameState->persistentArena = gameArena;

	glfwSetWindowUserPointer(window, CAST(void *)(gameState));

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
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		asteroid_gameUpdateAndRender(gameState, &memory, windowSize,
		                             secondsElapsed);
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
			         "Dengine | %f ms/f | %f fps | Entity Count: %d",
			         msPerFrame, framesPerSecond, gameState->world.entityIndex);

			glfwSetWindowTitle(window, textBuffer);
			titleUpdateFrequencyInSeconds = 0.5f;
		}

		startTime = endTime;
	}

	glfwTerminate();
	return 0;
}
