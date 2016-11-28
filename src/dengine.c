#include "Dengine/AssetManager.h"
#include "Dengine/Asteroid.h"
#include "Dengine/Common.h"
#include "Dengine/Debug.h"
#include "Dengine/Math.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/OpenGL.h"

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

	switch (button)
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

INTERNAL void setGlfwWindowHints()
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
}

i32 main(void)
{
	/*
	 **************************
	 * INIT APPLICATION WINDOW
	 **************************
	 */
	glfwInit();
	setGlfwWindowHints();

	OptimalArrayV2 vidList = {0};
	common_optimalArrayV2Create(&vidList);

	i32 windowWidth  = 0;
	i32 windowHeight = 0;
	{ // Query Computer Video Resolutions

		i32 numMonitors;
		GLFWmonitor **monitors      = glfwGetMonitors(&numMonitors);
		GLFWmonitor *primaryMonitor = monitors[0];

		i32 numModes;
		const GLFWvidmode *modes = glfwGetVideoModes(primaryMonitor, &numModes);

		i32 targetRefreshHz   = 60;
		f32 targetWindowRatio = 16.0f / 9.0f;

		i32 targetPixelDensity   = 1280 * 720;
		i32 minPixelDensityDelta = 100000000;

		printf("== Supported video modes ==\n");
		for (i32 i = 0; i < numModes; i++)
		{
			GLFWvidmode mode = modes[i];
			printf("width: %d, height: %d, rgb: %d, %d, %d, refresh: %d\n",
			       mode.width, mode.height, mode.redBits, mode.greenBits,
			       mode.blueBits, mode.refreshRate);

			if (mode.refreshRate == targetRefreshHz)
			{
				i32 result = common_optimalArrayV2Push(
				    &vidList, V2i(mode.width, mode.height));

				if (result)
				{
					printf(
					    "common_optimalArrayV2Push(): Failed error code %d\n",
					    result);
					ASSERT(INVALID_CODE_PATH);
				}

				f32 sizeRatio = (f32)mode.width / (f32)mode.height;
				f32 delta     = targetWindowRatio - sizeRatio;
				if (delta < 0.1f)
				{
					i32 pixelDensity = mode.width * mode.height;
					i32 densityDelta = ABS((pixelDensity - targetPixelDensity));

					if (densityDelta < minPixelDensityDelta)
					{
						minPixelDensityDelta = densityDelta;
						windowWidth          = mode.width;
						windowHeight         = mode.height;
					}
				}
			}
		}
		printf("==  ==\n");
		ASSERT(vidList.index > 0);
	}

	if (windowWidth == 0 || windowHeight == 0)
	{
		// NOTE(doyle): In this case just fallback to some value we hope is safe
		windowWidth  = 800;
		windowHeight = 600;
	}

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
	Memory memory              = {0};
	MemoryIndex persistentSize = MEGABYTES(32);
	MemoryIndex transientSize  = MEGABYTES(64);

	memory.persistentSize = persistentSize;
	memory.persistent     = PLATFORM_MEM_ALLOC_(NULL, persistentSize, u8);

	memory.transientSize = transientSize;
	memory.transient     = PLATFORM_MEM_ALLOC_(NULL, transientSize, u8);

	MemoryArena_ gameArena = {0};
	memory_arenaInit(&gameArena, memory.persistent, memory.persistentSize);

	GameState *gameState       = MEMORY_PUSH_STRUCT(&gameArena, GameState);
	gameState->persistentArena = gameArena;

	glfwSetWindowUserPointer(window, CAST(void *)(gameState));

	{ // Load game icon
		i32 width, height;
		char *iconPath = "data/textures/Asteroids/icon.png";
		u8 *pixels = asset_imageLoad(&width, &height, NULL, iconPath, FALSE);

		if (pixels)
		{
			GLFWimage image = {width, height, pixels};
			glfwSetWindowIcon(window, 1, &image);
			asset_imageFree(pixels);
		}
	}
	gameState->input.resolutionList = &vidList;

	/*
	 *******************
	 * GAME LOOP
	 *******************
	 */
	f32 startTime      = CAST(f32)(glfwGetTime());
	f32 secondsElapsed = 0.0f; // Time between current frame and last frame

#if 0
	// TODO(doyle): Get actual monitor refresh rate
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
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		asteroid_gameUpdateAndRender(gameState, &memory, windowSize,
		                             secondsElapsed);
		GL_CHECK_ERROR();

		/* Swap the buffers */
		glfwSwapBuffers(window);

		f32 endTime    = CAST(f32) glfwGetTime();
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

			i32 entityCount = 0;
			GameWorldState *world =
			    ASTEROID_GET_STATE_DATA(gameState, GameWorldState);

			if (world) entityCount = world->entityIndex;

			char textBuffer[256];
			snprintf(textBuffer, ARRAY_COUNT(textBuffer),
			         "Dengine | %f ms/f | %f fps | Entity Count: %d",
			         msPerFrame, framesPerSecond, entityCount);

			glfwSetWindowTitle(window, textBuffer);
			titleUpdateFrequencyInSeconds = 0.5f;
		}

		startTime = endTime;

		StartMenuState *menuState =
		    ASTEROID_GET_STATE_DATA(gameState, StartMenuState);
		if (menuState)
		{
			if (menuState->newResolutionRequest)
			{
				i32 index  = menuState->resStringDisplayIndex;
				windowSize = gameState->input.resolutionList->ptr[index];

				glfwSetWindowSize(window, (i32)windowSize.w, (i32)windowSize.h);
				glViewport(0, 0, (i32)windowSize.w, (i32)windowSize.h);

				menuState->newResolutionRequest = FALSE;
			}
		}
	}

	glfwTerminate();
	return 0;
}
