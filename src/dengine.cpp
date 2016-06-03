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

	i32 width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	glfwSetKeyCallback(window, key_callback);

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
		// Positions          Colors             Texture Coords
		+0.5f, +0.5f, +0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, // Top Right
		+0.5f, -0.5f, +0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f, // Bottom Right
	    -0.5f, -0.5f, +0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Bottom left
	    -0.5f, +0.5f, +0.0f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f, // Top left
	};

	GLuint indices[] = {
	    0, 1, 3, // First triangle
	    1, 2, 3, // First triangle
	};

	GLuint vbo, vao, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	// 1. Bind vertex array object
	glBindVertexArray(vao);

	// 2. Bind and set vertex buffer(s) and attribute pointer(s)
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

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


	// 4. Unbind to prevent mistakes
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	/* Main game loop */
	while (!glfwWindowShouldClose(window))
	{
		/* Check and call events */
		glfwPollEvents();

		/* Rendering commands here*/
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shader.use();

		/* Math */
		glm::mat4 trans;
		trans = glm::translate(trans, glm::vec3(0.5f, -0.5f, 0.0f));
		GLfloat rotation = glm::radians((GLfloat)glfwGetTime() * 50.0f);
		trans = glm::rotate(trans, rotation,
		                    glm::vec3(0.0, 0.0, 1.0));
		GLuint transformLoc =
		    glGetUniformLocation(shader.mProgram, "transform");
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(glGetUniformLocation(shader.mProgram, "ourTexture"), 0);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		/* Swap the buffers */
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);

	glfwTerminate();
	return 0;
}
