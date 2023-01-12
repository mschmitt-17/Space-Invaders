#include <cstdio>
#include <cstdint>
#include </Users/18472/GL/GLEW/glew-2.1.0/include/GL/glew.h> //GLEW is a loading library, since OpenGL functions need to be declared and loaded explicitly at runtime
#include </Users/18472/GL/GLFW/glfw-3.3.8.bin.WIN32/include/GLFW/glfw3.h>

#include "buffer.h"
#include "shader.h"

struct Alien {
	size_t x, y; //x, y are pixels from the bottom left corner of the screen
	uint8_t type;
};

struct Player {
	size_t x, y;
	size_t life; //size_t used to represent the size of an object (so size of life?)
};

struct Game { //this struct holds all game related variables
	size_t width, height;
	size_t num_aliens;
	Alien* aliens;
	Player player;
};

struct SpriteAnimation {
	bool loop; //loop over animation or only play it once
	size_t num_frames;
	size_t frame_duration; //time between successive frames
	size_t time; //time in current animation instance
	Sprite** frames;
};

void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

int main(int argc, char* argv[]) {
	const size_t buffer_width = 224;
	const size_t buffer_height = 256;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		return -1;
	}

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);	//|
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);					//|--> tells GLFW we want context that is at least 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);					//| definition: "a task context is the minimal set of data used by a task that must be saved to allow a yask to be interrupted and later continued"
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);			//|
	GLFWwindow* window = glfwCreateWindow(buffer_width, buffer_height, "Space Invaders", NULL, NULL);

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

	glClearColor(1.0, 0.0, 0.0, 1.0); //set buffer clear color to red

	Buffer buffer;
	buffer.width = buffer_width;
	buffer.height = buffer_height;
	buffer.data = new uint32_t[buffer.width * buffer.height];

	buffer_clear(&buffer, 0);
	

	GLuint buffer_texture; //texture is object
	glGenTextures(1, &buffer_texture);
	glBindTexture(GL_TEXTURE_2D, buffer_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, buffer.width, buffer.height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data); //last 3: each pixel in rgba format, represented as 4 unsigend 8 bit intgers
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //|
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //|-->tell gpu not to apply any smoothing when reading pixels
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //|
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); //|-->use value at edges if trying to read beyond texture edges

	GLuint fullscreen_triangle_vao;					//|
	glGenVertexArrays(1, &fullscreen_triangle_vao);	//|--> vertex array object(vao), stores format of vertex data and data itself

	//Shaders: Vertex shaders handle vertex data (describes position of something in 2D/3D space), most often used to give objects screen-space coordinates.
	//		   Fragment shader processes fragments from processing of vertex shader output, outputting value(s) such as a depth value, color value, etc.

	const char* fragment_shader =
		"\n"
		"#version 330\n"
		"\n"
		"uniform sampler2D buffer;\n"
		"noperspective in vec2 TexCoord;\n"
		"\n"
		"out vec3 outColor;\n"
		"\n"
		"void main(void){\n"
		"	outColor = texture(buffer, TexCoord).rgb;\n"
		"}\n";

	const char* vertex_shader = //want to generate a quad that covers the screen. This is a trick that allows us to generate a fullscreen triangle instead
		"\n"
		"#version 330\n"
		"\n"
		"noperspective out vec2 TexCoord;\n" //texcoord is output of vertex shader (vec2 is type)
		"\n"
		"void main(void){\n"
		"\n"
		"	TexCoord.x = (gl_VertexID == 2)? 2.0: 0.0;\n"
		"	TexCoord.y = (gl_VertexID == 1)? 2.0: 0.0;\n"
		"	\n"
		"	gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);\n"
		"}\n";

	GLuint shader_id = glCreateProgram(); //creates shader program

	{ //vertex shader
		GLuint shader_vp = glCreateShader(GL_VERTEX_SHADER); //creates individual shader

		glShaderSource(shader_vp, 1, &vertex_shader, 0); //shader source is vertex shader array defined above
		glCompileShader(shader_vp);
		validate_shader(shader_vp, vertex_shader);
		glAttachShader(shader_id, shader_vp); //attach to shader program

		glDeleteShader(shader_vp);
	}

	{ //fragment shader
		GLuint shader_fp = glCreateShader(GL_FRAGMENT_SHADER);

		glShaderSource(shader_fp, 1, &fragment_shader, 0);
		glCompileShader(shader_fp);
		validate_shader(shader_fp, fragment_shader);
		glAttachShader(shader_id, shader_fp);

		glDeleteShader(shader_fp);
	}

	glLinkProgram(shader_id); //links vertex and fragment shaders

	if (!validate_program(shader_id)) {
		fprintf(stderr, "Error while validating shader.\n");
		glfwTerminate();
		glDeleteVertexArrays(1, &fullscreen_triangle_vao);
		delete[] buffer.data; //delete array of buffer data
		return -1;
	}

	glUseProgram(shader_id);

	GLint location = glGetUniformLocation(shader_id, "buffer"); //get uniform (global shader variable) for location
	glUniform1i(location, 0); //attach uniform from buffer to texture unit 0

	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(fullscreen_triangle_vao);
	
	Sprite player_sprite;
	player_sprite.width = 11;
	player_sprite.height = 7;
	player_sprite.data = new uint8_t[77]{
		0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	};

	Sprite alien_sprite0;
	alien_sprite0.width = 11;
	alien_sprite0.height = 8;
	alien_sprite0.data = new uint8_t[88]{
		0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1,
		1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1,
		0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0
	};

	Sprite alien_sprite1;
	alien_sprite1.width = 11;
	alien_sprite1.height = 8;
	alien_sprite1.data = new uint8_t[88]{
		0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0,
		1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1,
		1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1,
		1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0
	};

	SpriteAnimation* alien_animation = new SpriteAnimation;

	alien_animation->loop = true;
	alien_animation->num_frames = 2;
	alien_animation->frame_duration = 10;
	alien_animation->time = 0;
	alien_animation->frames = new Sprite * [2];
	alien_animation->frames[0] = &alien_sprite0;
	alien_animation->frames[1] = &alien_sprite1;

	glfwSwapInterval(1); //syncs video card rate with monitor refresh rate (around 60 fps)

	Game game;
	game.width = buffer_width;
	game.height = buffer_height;
	game.num_aliens = 55;
	game.aliens = new Alien[game.num_aliens];

	game.player.x = 112 - 5;
	game.player.y = 32;
	game.player.life = 3;

	for (size_t yi = 0; yi < 5; yi++) { //iterate over every entry in aliens[], setting position
		for (size_t xi = 0; xi < 11; xi++) {
			game.aliens[yi * 11 + xi].x = 16 * xi + 20;
			game.aliens[yi * 11 + xi].y = 17 * yi + 128;
		}
	}

	int player_move_dir = 1; //arbitrary player movement

	uint32_t clear_color = rgb_to_uint32(0, 128, 0);

	while (!glfwWindowShouldClose(window)) {
		buffer_clear(&buffer, clear_color); //sets color to clear color, which is green

		for (size_t ai = 0; ai < game.num_aliens; ai++) {
			const Alien& alien = game.aliens[ai];
			size_t current_frame = alien_animation->time / alien_animation->frame_duration;
			const Sprite& sprite = *alien_animation->frames[current_frame];
			buffer_sprite_draw(&buffer, sprite, alien.x, alien.y, rgb_to_uint32(128, 0, 0));
		}

		buffer_sprite_draw(&buffer, player_sprite, game.player.x, game.player.y, rgb_to_uint32(128, 0, 0));
		
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer.width, buffer.height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data);
		
		glDrawArrays(GL_TRIANGLES, 0, 3);
		
		glfwSwapBuffers(window); //front buffer is used for displaying an image, while back buffer is used for drawing. They are swapped at each iteration (front displays, back is updated with new actions)
		
		glfwPollEvents(); //poll any new events that might have happened

		alien_animation->time++;
		if (alien_animation->time == alien_animation->num_frames * alien_animation->frame_duration) {
			if (alien_animation->loop) { //set time back to 0 if animation loops, otherwise delete it
				alien_animation->time = 0;
			}
			else {
				delete alien_animation;
				alien_animation = nullptr;
			}
		}

		if (game.player.x + player_sprite.width + player_move_dir >= game.width - 1) { //keep player within bounds if collision is about to happen
			game.player.x = game.width - player_sprite.width - player_move_dir - 1;
			player_move_dir *= -1;
		}
		else if ((int)game.player.x + player_move_dir <= 0) {
			game.player.x = 0;
			player_move_dir *= -1;
		} else game.player.x += player_move_dir;
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	glDeleteVertexArrays(1, &fullscreen_triangle_vao);

	delete[] alien_sprite0.data;
	delete[] alien_sprite1.data;
	delete[] player_sprite.data;
	delete[] buffer.data;

	return 0;
}