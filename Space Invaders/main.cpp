#include <cstdio>
#include </Users/18472/GL/GLEW/glew-2.1.0/include/GL/glew.h> //GLEW is a loading library, since OpenGL functions need to be declared and loaded explicitly at runtime
#include </Users/18472/GL/GLFW/glfw-3.3.8.bin.WIN32/include/GLFW/glfw3.h>


void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

int main() {
	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		return -1;
	}

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);	//|
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);					//|--> tells GLFW we want context that is at least 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);					//| definition: "a task context is the minimal set of data used by a task that must be saved to allow a yask to be interrupted and later continued"
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);			//|
	GLFWwindow* window = glfwCreateWindow(640, 480, "Space Invaders", NULL, NULL);

	if (!window) {
		glfwTerminate(); //let glfw clear resources if there are any problems opening window
		return -1;
	}
	glfwMakeContextCurrent(window); //make openGL calls after this apply to current context

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		fprintf(stderr, "Error initializing GLEW.\n");
		glfwTerminate();
		return -1;
	}

	int glVersion[2] = { -1, 1 };
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);	// |---> since OpenGL is API specific, we don't know what version we have
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);	// |
	printf("Using OpenGL: %d.%d\n", glVersion[0], glVersion[1]);
	//defn: a buffer refers to a portion of RAM used to hold a temporary image
	glClearColor(1.0, 0.0, 0.0, 1.0); //set buffer clear color to red
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(window); //front buffer is used for displaying an image, while back buffer is used for drawing. They are swapped at each iteration (front displays, back is updated with new actions)
		glfwPollEvents(); //poll any new events that might have happened
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}