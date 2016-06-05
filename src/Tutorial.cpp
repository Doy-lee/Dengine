#if 0
#include <Dengine/OpenGL.h>
#include <Dengine/Common.h>
#include <Dengine/Shader.h>
#include <Dengine/AssetManager.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cstdint>
#include <fstream>
#include <string>

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

f32 lastX = 400;
f32 lastY = 300;
f32 yaw   = -90.0f;
f32 pitch = 0.0f;
f32 fov   = 45.0f;
b32 firstMouse;

b32 keys[1024];

enum BytesPerPixel
{
	Greyscale      = 1,
	GreyscaleAlpha = 2,
	RGB            = 3,
	RGBA           = 4,
};

INTERNAL GLint getGLFormat(BytesPerPixel bytesPerPixel, b32 srgb)
{
	switch (bytesPerPixel)
	{
		case Greyscale:
			return GL_LUMINANCE;
		case GreyscaleAlpha:
			return GL_LUMINANCE_ALPHA;
		case RGB:
			return (srgb ? GL_SRGB : GL_RGB);
		case RGBA:
			return (srgb ? GL_SRGB_ALPHA : GL_RGBA);
		default:
			// TODO(doyle): Invalid
		    std::cout << "getGLFormat() invalid bytesPerPixel: "
		              << bytesPerPixel << std::endl;
		    return GL_LUMINANCE;
	}
}

void doMovement(f32 deltaTime)
{
	f32 cameraSpeed = 5.00f * deltaTime;
	if (keys[GLFW_KEY_W])
		cameraPos += (cameraSpeed * cameraFront);
	if (keys[GLFW_KEY_S])
		cameraPos -= (cameraSpeed * cameraFront);
	if (keys[GLFW_KEY_A])
		cameraPos -=
		    glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (keys[GLFW_KEY_D])
		cameraPos +=
		    glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = TRUE;
		else if (action == GLFW_RELEASE)
			keys[key] = FALSE;
	}
}

void mouse_callback(GLFWwindow *window, double xPos, double yPos)
{
	if (firstMouse)
	{
		lastX = (f32)xPos;
		lastY = (f32)yPos;
		firstMouse = false;
	}

	f32 xOffset = (f32)xPos - lastX;
	f32 yOffset = lastY - (f32)yPos;
	lastX = (f32)xPos;
	lastY = (f32)yPos;

	f32 sensitivity = 0.05f;
	xOffset *= sensitivity;
	yOffset *= sensitivity;

	yaw   += xOffset;
	pitch += yOffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	else if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x     = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
	front.y     = sin(glm::radians(pitch));
	front.z     = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow *window, double xOffset, double yOffset)
{
	if (fov >= 1.0f && fov <= 45.0f)
		fov -= (f32)yOffset;
	else if (fov <= 1.0f)
		fov = 1.0f;
	else // fov >= 45.0f
		fov = 45.0f;
}

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:
			error = "INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			error = "INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			error = "INVALID_OPERATION";
			break;
		case GL_STACK_OVERFLOW:
			error = "STACK_OVERFLOW";
			break;
		case GL_STACK_UNDERFLOW:
			error = "STACK_UNDERFLOW";
			break;
		case GL_OUT_OF_MEMORY:
			error = "OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			error = "INVALID_FRAMEBUFFER_OPERATION";
			break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	glm::ivec2 windowSize = glm::ivec2(800, 600);

	GLFWwindow *window = glfwCreateWindow(windowSize.x, windowSize.y,
	                                      "LearnOpenGL", nullptr, nullptr);

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
		std::cout << "glewInit() failed: Failed to initialise GLEW"
		          << std::endl;
		return -1;
	}

	glm::ivec2 frameBufferSize;
	glfwGetFramebufferSize(window, &frameBufferSize.x, &frameBufferSize.y);
	glViewport(0, 0, frameBufferSize.x, frameBufferSize.y);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	firstMouse = TRUE;

	glEnable(GL_DEPTH_TEST | GL_CULL_FACE);
	glCullFace(GL_BACK);

	/* Initialise shaders */
	Dengine::AssetManager assetManager;
	i32 result = 0;

	result = assetManager.loadShaderFiles("data/shaders/default.vert.glsl",
	                                      "data/shaders/default.frag.glsl",
	                                      "default");
	if (result) return result;

	/* Load a texture */
	result = assetManager.loadTextureImage("data/textures/container.jpg",
	                                       "container");
	if (result) return result;

	result = assetManager.loadTextureImage("data/textures/wall.jpg",
	                                       "wall");
	if (result) return result;

	Dengine::Texture *containerTex = assetManager.getTexture("container");
	Dengine::Texture *wallTex = assetManager.getTexture("wall");
	Dengine::Shader *shader = assetManager.getShader("default");

	/* Create OGL Vertex objects */
	GLfloat sceneVertex[] = {
		// Positions          Colors             Texture Coords
		// Floor
		+0.5f, +0.5f, +0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, // Top Right
		+0.5f, -0.5f, +0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f, // Bottom Right
		-0.5f, -0.5f, +0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Bottom left
		-0.5f, +0.5f, +0.0f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f, // Top left

		// Wall
		+0.5f, +0.5f, +0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, // Top Right
		+0.5f, -0.5f, +0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f, // Bottom Right
		-0.5f, -0.5f, +0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Bottom left
		-0.5f, +0.5f, +0.0f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f, // Top left
	};

	GLuint sceneIndices[] = {
		// Floor
	    0, 3, 1, // First triangle
	    1, 2, 3, // Second triangle

		// Wall
	    4, 7, 5, // First triangle
	    5, 6, 7, // Second triangle
	};

	GLuint vbo, vao;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	GLuint ebo;
	glGenBuffers(1, &ebo);

	// 1. Bind vertex array object
	glBindVertexArray(vao);

	// 2. Bind and set vertex buffer(s) and attribute pointer(s)
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sceneVertex), sceneVertex,
	             GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sceneIndices), sceneIndices,
	             GL_STATIC_DRAW);

	const i32 numPos      = 3;
	const i32 numColors   = 3;
	const i32 numTexCoord = 2;

	const GLint vertexSize =
	    (numPos + numColors + numTexCoord) * sizeof(GLfloat);

	const GLint vertByteOffset     = 0;
	const GLint colorByteOffset    = vertByteOffset + (numPos * sizeof(GLfloat));
	const GLint texCoordByteOffset = colorByteOffset + (numColors * sizeof(GLfloat));

	glVertexAttribPointer(0, numPos, GL_FLOAT, GL_FALSE, vertexSize,
	                      (GLvoid *)vertByteOffset);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, numColors, GL_FLOAT, GL_FALSE, vertexSize,
	                      (GLvoid *)colorByteOffset);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, numTexCoord, GL_FLOAT, GL_FALSE, vertexSize,
	                      (GLvoid *)texCoordByteOffset);
	glEnableVertexAttribArray(2);

	// 4. Unbind to prevent mistakes
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	f32 deltaTime = 0.0f; // Time between current frame and last frame
	f32 lastFrame = 0.0f; // Time of last frame

	/* Main game loop */
	while (!glfwWindowShouldClose(window))
	{
		f32 currentFrame = (f32)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		/* Check and call events */
		glfwPollEvents();
		doMovement(deltaTime);

		/* Rendering commands here*/
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* Bind textures */
		glActiveTexture(GL_TEXTURE0);
		shader->use();

		/* Camera/View transformation */
		glm::mat4 model;
		model = glm::translate(model, glm::vec3(0.0f, -0.3f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		glm::mat4 view;
		// NOTE(doyle): Lookat generates the matrix for camera coordinate axis
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		/* Projection */
		glm::mat4 projection;
		f32 aspectRatio = static_cast<f32>(frameBufferSize.x) /
		                  static_cast<f32>(frameBufferSize.y);
		projection = glm::perspective(fov, aspectRatio, 0.1f, 100.0f);

		/* Get shader uniform locations */
		GLuint modelLoc = glGetUniformLocation(shader->mProgram, "model");
		GLuint viewLoc  = glGetUniformLocation(shader->mProgram, "view");
		GLuint projectionLoc =
		    glGetUniformLocation(shader->mProgram, "projection");

		/* Pass matrices to the shader */
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(vao);

		glBindTexture(GL_TEXTURE_2D, containerTex->mId);
		glUniform1i(glGetUniformLocation(shader->mProgram, "ourTexture"), 0);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glBindTexture(GL_TEXTURE_2D, wallTex->mId);
		glUniform1i(glGetUniformLocation(shader->mProgram, "ourTexture"), 0);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (GLvoid *)&sceneVertex[4]);

		glBindVertexArray(0);

		/* Swap the buffers */
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
#if 0
	glDeleteBuffers(1, &ebo);
#endif

	glfwTerminate();
	return 0;
}
#endif
