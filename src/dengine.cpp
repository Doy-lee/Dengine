#include <Dengine/OpenGL.h>
#include <Dengine/Common.h>
#include <Dengine/Shader.h>

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cstdint>
#include <fstream>
#include <string>

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

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow *window =
	    glfwCreateWindow(800, 600, "LearnOpenGL", nullptr, nullptr);

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

	i32 screenWidth, screenHeight;
	glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
	glViewport(0, 0, screenWidth, screenHeight);

	glfwSetKeyCallback(window, key_callback);

	glEnable(GL_DEPTH_TEST);

	/* Initialise shaders */
	Dengine::Shader shader = Dengine::Shader("data/shaders/default.vert.glsl",
	                                         "data/shaders/default.frag.glsl");
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Set texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//  Set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	/* Load a texture */
	i32 imgWidth, imgHeight, bytesPerPixel;
	stbi_set_flip_vertically_on_load(TRUE);
	u8 *image = stbi_load("data/textures/container.jpg", &imgWidth, &imgHeight, &bytesPerPixel, 0);

	if (!image)
	{
		std::cerr << "stdbi_load() failed: " << stbi_failure_reason()
		          << std::endl;
	}

	glTexImage2D(GL_TEXTURE_2D, 0,
	             getGLFormat((BytesPerPixel)bytesPerPixel, FALSE), imgWidth,
	             imgHeight, 0, getGLFormat((BytesPerPixel)bytesPerPixel, FALSE),
	             GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);

	/* Create OGL Vertex objects */
	GLfloat vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};

	glm::vec3 cubePositions[] = {
		glm::vec3( 0.0f,  0.0f,  0.0f),
		glm::vec3( 2.0f,  5.0f, -15.0f),
		glm::vec3(-1.5f, -2.2f, -2.5f),
		glm::vec3(-3.8f, -2.0f, -12.3f),
		glm::vec3( 2.4f, -0.4f, -3.5f),
		glm::vec3(-1.7f,  3.0f, -7.5f),
		glm::vec3( 1.3f, -2.0f, -2.5f),
		glm::vec3( 1.5f,  2.0f, -2.5f),
		glm::vec3( 1.5f,  0.2f, -1.5f),
		glm::vec3(-1.3f,  1.0f, -1.5f)
	};

#if 0
	GLfloat vertices[] = {
		// Positions          Colors             Texture Coords
		+0.5f, +0.5f, +0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, // Top Right
		+0.5f, -0.5f, +0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f, // Bottom Right
		-0.5f, -0.5f, +0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Bottom left
		-0.5f, +0.5f, +0.0f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f, // Top left
	};
#endif
	GLuint indices[] = {
	    0, 1, 3, // First triangle
	    1, 2, 3, // First triangle
	};


	GLuint vbo, vao;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
#if 0
	GLuint ebo;
	glGenBuffers(1, &ebo);
#endif

	// 1. Bind vertex array object
	glBindVertexArray(vao);

	// 2. Bind and set vertex buffer(s) and attribute pointer(s)
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

#if 0
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

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
#endif

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
	                      (GLvoid *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
	                      (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
		

	// 4. Unbind to prevent mistakes
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glm::vec3 cameraPos       = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraTarget    = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);

	glm::vec3 upVec       = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 cameraRight = glm::normalize(glm::cross(upVec, cameraPos));
	glm::vec3 cameraUp    = glm::cross(cameraDirection, cameraRight);

	glm::mat4 view;
	// NOTE(doyle): Lookat generates the matrix for camera coordinate axis
	view = glm::lookAt(cameraPos, cameraTarget, upVec);

	/* Main game loop */
	while (!glfwWindowShouldClose(window))
	{
		/* Check and call events */
		glfwPollEvents();

		/* Rendering commands here*/
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();

		/* Camera */
		GLfloat radius = 10.0f;
		GLfloat camX   = sin(glfwGetTime()) * radius;
		GLfloat camZ   = cos(glfwGetTime()) * radius;
		view = glm::lookAt(glm::vec3(camX, 0.0, camZ), cameraTarget, upVec);

		/* Math */
		view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

		glm::mat4 projection;
		projection =
		    glm::perspective(glm::radians(45.0f), ((f32)screenWidth / (f32)screenHeight), 0.1f, 100.0f);

		
		GLuint modelLoc = glGetUniformLocation(shader.mProgram, "model");
		GLuint viewLoc = glGetUniformLocation(shader.mProgram, "view");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		GLuint projectionLoc = glGetUniformLocation(shader.mProgram, "projection");
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(glGetUniformLocation(shader.mProgram, "ourTexture"), 0);

		glBindVertexArray(vao);
		// glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		for (GLuint i = 0; i < 10; i++)
		{
			glm::mat4 model;
			model = glm::translate(model, cubePositions[i]);

			GLfloat angle = glm::radians(20.0f * i);
			model = glm::rotate(model, angle, glm::vec3(1.0f, 0.3f, 0.5f));

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
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
