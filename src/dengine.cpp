#if 1
#include <Dengine/AssetManager.h>
#include <Dengine/Common.h>
#include <Dengine/OpenGL.h>
#include <Dengine/Renderer.h>
#include <Dengine/Shader.h>

#include <WorldTraveller/WorldTraveller.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	WorldTraveller::Game *game =
	    static_cast<WorldTraveller::Game *>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < WorldTraveller::NUM_KEYS)
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
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	glm::ivec2 windowSize = glm::ivec2(1280, 720);

	GLFWwindow *window = glfwCreateWindow(windowSize.x, windowSize.y, "Dengine", nullptr, nullptr);

	if (!window)
	{
		std::cout << "glfwCreateWindow() failed: Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	/* Make GLEW use more modern technies for OGL on core profile*/
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "glewInit() failed: Failed to initialise GLEW" << std::endl;
		return -1;
	}
	// NOTE(doyle): glewInit() bug that sets the gl error flag after init
	// regardless of success. Catch it once by calling glGetError
	glGetError();

	glm::ivec2 frameBufferSize;
	glfwGetFramebufferSize(window, &frameBufferSize.x, &frameBufferSize.y);
	glViewport(0, 0, frameBufferSize.x, frameBufferSize.y);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);

	WorldTraveller::Game game = WorldTraveller::Game(frameBufferSize.x, frameBufferSize.y);
	game.init();

	glfwSetWindowUserPointer(window, static_cast<void *>(&game));

	f32 secondsElapsed = 0.0f; // Time between current frame and last frame

	// TODO(doyle): Get actual monitor refresh rate
	i32 monitorRefreshHz = 60;
	f32 targetSecondsPerFrame  = 1.0f / static_cast<f32>(monitorRefreshHz);
	f32 startTime = static_cast<f32>(glfwGetTime());

	//glfwSwapInterval(1);

	/* Main game loop */
	while (!glfwWindowShouldClose(window))
	{

		/* Check and call events */
		glfwPollEvents();

		game.update(secondsElapsed);

		/* Rendering commands here*/
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		game.render();
		glCheckError();

		/* Swap the buffers */
		glfwSwapBuffers(window);

		f32 endTime = static_cast<f32>(glfwGetTime());
		secondsElapsed = endTime - startTime;

		// TODO(doyle): Busy waiting, should sleep
		while (secondsElapsed < targetSecondsPerFrame)
		{
			endTime        = static_cast<f32>(glfwGetTime());
			secondsElapsed = endTime - startTime;
		}

		LOCAL_PERSIST f32 titleUpdateFrequencyInSeconds = 0.5f;

		titleUpdateFrequencyInSeconds -= secondsElapsed;
		if (titleUpdateFrequencyInSeconds <= 0)
		{
			f32 msPerFrame      = secondsElapsed * 1000.0f;
			f32 framesPerSecond = 1.0f / secondsElapsed;
			std::stringstream ss;
			ss << "Dengine | " << msPerFrame << " ms/f | " << framesPerSecond << " fps";
			glfwSetWindowTitle(window, ss.str().c_str());
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
