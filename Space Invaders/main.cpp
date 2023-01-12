#include <cstdio>
#include <cstdint>
#include </Users/18472/GL/GLEW/glew-2.1.0/include/GL/glew.h> //GLEW is a loading library, since OpenGL functions need to be declared and loaded explicitly at runtime
#include </Users/18472/GL/GLFW/glfw-3.3.8.bin.WIN32/include/GLFW/glfw3.h>

#include "buffer.h"
#include "shader.h"

#define GAME_MAX_BULLETS	3

struct Alien {
	size_t x, y; //x, y are pixels from the bottom left corner of the screen
	uint8_t type;
};

enum AlienType : uint8_t {
	ALIEN_DEAD = 0,
	ALIEN_TYPE_A = 1,
	ALIEN_TYPE_B = 2,
	ALIEN_TYPE_C = 3
};

struct Player {
	size_t x, y;
	size_t life; //size_t used to represent the size of an object (so size of life?)
};

struct Bullet {
	size_t x, y;
	int dir; // - is towards player, + is towards aliens
};

struct Game { //this struct holds all game related variables
	size_t width, height;
	size_t num_aliens;
	size_t num_bullets;
	Alien* aliens;
	Player player;
	Bullet bullets[GAME_MAX_BULLETS];
};

struct SpriteAnimation {
	bool loop; //loop over animation or only play it once
	size_t num_frames;
	size_t frame_duration; //time between successive frames
	size_t time; //time in current animation instance
	Sprite** frames;
};

bool game_running = false; //game_running will allow the game to be quit when the esc key is pressed
int move_dir = 0; //variable for player movement direction (+1 is right arrow, -1 is left arrow)
bool fire_pressed = 0; //boolean for if firing button (space) was pressed

bool sprite_overlap_check(const Sprite& sp_a, size_t x_a, size_t y_a, const Sprite& sp_b, size_t x_b, size_t y_b) {
	if (x_a < x_b + sp_b.width && x_a + sp_a.width > x_b && y_a < y_b + sp_b.height && y_a + sp_a.height > y_b) {
		return true;
	}
	return false;
}

void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) { //mods are shift, control, etc. and we don't use scancode
	switch (key) {
		case GLFW_KEY_ESCAPE:
			if (action == GLFW_PRESS) game_running = false;
			break;
		case GLFW_KEY_RIGHT:
			if (action == GLFW_PRESS) move_dir += 1; //if both keys are pressed, we want the player to not move so we add instead of setting
			else if (action == GLFW_RELEASE) move_dir -= 1; //want player to stop of key is released
			break;
		case GLFW_KEY_LEFT:
			if (action == GLFW_PRESS) move_dir -= 1;
			else if (action == GLFW_RELEASE) move_dir += 1;
			break;
		case GLFW_KEY_SPACE:
			if (action == GLFW_RELEASE) fire_pressed = true;
			break;
		default:
			break;
	}
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
	glfwSetKeyCallback(window, key_callback);

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

	Sprite alien_sprites[6]; //two for each alien

	alien_sprites[0].width = 8;
	alien_sprites[0].height = 8;
	alien_sprites[0].data = new uint8_t[64]{
		0, 0, 0, 1, 1, 0, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 0, 1, 1, 0, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		0, 1, 0, 1, 1, 0, 1, 0,
		1, 0, 0, 0, 0, 0, 0, 1,
		0, 1, 0, 0, 0, 0, 1, 0
	};

	alien_sprites[1].width = 8;
	alien_sprites[1].height = 8;
	alien_sprites[1].data = new uint8_t[64]{
		0, 0, 0, 1, 1, 0, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 0, 1, 1, 0, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 1, 0, 0, 1, 0, 0,
		0, 1, 0, 1, 1, 0, 1, 0,
		1, 0, 1, 0, 0, 1, 0, 1
	};

	alien_sprites[2].width = 11;
	alien_sprites[2].height = 8;
	alien_sprites[2].data = new uint8_t[88]{
		0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1,
		1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1,
		0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0
	};

	alien_sprites[3].width = 11;
	alien_sprites[3].height = 8;
	alien_sprites[3].data = new uint8_t[88]{
		0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0,
		1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1,
		1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1,
		1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0
	};

	alien_sprites[4].width = 12;
	alien_sprites[4].height = 8;
	alien_sprites[4].data = new uint8_t[96]{
		0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0,
		0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1
	};

	alien_sprites[5].width = 12;
	alien_sprites[5].height = 8;
	alien_sprites[5].data = new uint8_t[96]{
		0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0,
		0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0
	};

	Sprite alien_death_sprite;
	alien_death_sprite.width = 13;
	alien_death_sprite.height = 7;
	alien_death_sprite.data = new uint8_t[91]{
		0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0,
		0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
		0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0,
		0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0
	};

	Sprite bullet_sprite;
	bullet_sprite.width = 1;
	bullet_sprite.height = 3;
	bullet_sprite.data = new uint8_t[3]{
		1,
		1,
		1
	};

	SpriteAnimation alien_animation[3];

	alien_animation[0].loop = true;
	alien_animation[0].num_frames = 2;
	alien_animation[0].frame_duration = 10;
	alien_animation[0].time = 0;
	alien_animation[0].frames = new Sprite * [2];
	alien_animation[0].frames[0] = &alien_sprites[0];
	alien_animation[0].frames[1] = &alien_sprites[1];

	alien_animation[1].loop = true;
	alien_animation[1].num_frames = 2;
	alien_animation[1].frame_duration = 10;
	alien_animation[1].time = 0;
	alien_animation[1].frames = new Sprite * [2];
	alien_animation[1].frames[0] = &alien_sprites[2];
	alien_animation[1].frames[1] = &alien_sprites[3];

	alien_animation[2].loop = true;
	alien_animation[2].num_frames = 2;
	alien_animation[2].frame_duration = 10;
	alien_animation[2].time = 0;
	alien_animation[2].frames = new Sprite * [2];
	alien_animation[2].frames[0] = &alien_sprites[4];
	alien_animation[2].frames[1] = &alien_sprites[5];

	glfwSwapInterval(1); //syncs video card rate with monitor refresh rate (around 60 fps)

	Game game;
	game.width = buffer_width;
	game.height = buffer_height;
	game.num_aliens = 55;
	game.num_bullets = 0;
	game.aliens = new Alien[game.num_aliens];

	game.player.x = 112 - 5;
	game.player.y = 32;
	game.player.life = 3;

	for (size_t yi = 0; yi < 5; yi++) { //iterate over every entry in aliens[], setting position
		for (size_t xi = 0; xi < 11; xi++) {
			game.aliens[yi * 11 + xi].x = 16 * xi + 20;
			game.aliens[yi * 11 + xi].y = 17 * yi + 128;
			if (yi > 3) game.aliens[yi * 11 + xi].type = ALIEN_TYPE_A;
			else if (yi > 1) game.aliens[yi * 11 + xi].type = ALIEN_TYPE_B;
			else game.aliens[yi * 11 + xi].type = ALIEN_TYPE_C;
		}
	}

	uint8_t* death_counters = new uint8_t[game.num_aliens];
	for (size_t i = 0; i < game.num_aliens; i++) {
		death_counters[i] = 10; //set every dead counter to 10, meaning death sprite will be shown for 10 frames before alien is removed
	}

	game_running = true;

	uint32_t clear_color = rgb_to_uint32(0, 128, 0);

	while (!glfwWindowShouldClose(window) && game_running) {
		buffer_clear(&buffer, clear_color); //sets color to clear color, which is green

		for (size_t ai = 0; ai < game.num_aliens; ai++) {
			if (!death_counters[ai]) continue; //if death counters is 0 we skip over the alien

			const Alien& alien = game.aliens[ai];
			if (alien.type == ALIEN_DEAD) {
				buffer_sprite_draw(&buffer, alien_death_sprite , alien.x, alien.y, rgb_to_uint32(128, 0, 0));
			} else {
				const SpriteAnimation& animation = alien_animation[alien.type - 1];
				size_t current_frame = animation.time / animation.frame_duration;
				const Sprite& sprite = *animation.frames[current_frame];
				buffer_sprite_draw(&buffer, sprite, alien.x, alien.y, rgb_to_uint32(128, 0, 0));
			}
		}

		for (size_t bi = 0; bi < game.num_bullets; bi++) { //draw a num_bullets bullets
			const Bullet& bullet = game.bullets[bi];
			const Sprite& sprite = bullet_sprite;
			buffer_sprite_draw(&buffer, sprite, bullet.x, bullet.y, rgb_to_uint32(128, 0, 0));
		}

		for (size_t bi = 0; bi < game.num_bullets;) { //remove any projectiles that move out of the area by overwriting it with last element in array
			game.bullets[bi].y += game.bullets[bi].dir;
			if (game.bullets[bi].y >= game.height || game.bullets[bi].y < bullet_sprite.height) {
				game.bullets[bi] = game.bullets[game.num_bullets - 1];
				game.num_bullets--;
				continue;
			}
			for (size_t ai = 0; ai < game.num_aliens; ai++) {
				const Alien& alien = game.aliens[ai];
				if (alien.type == ALIEN_DEAD) continue;

				const SpriteAnimation& animation = alien_animation[alien.type - 1];
				size_t current_frame = animation.time / animation.frame_duration;
				const Sprite& alien_sprite = *animation.frames[current_frame];
				bool overlap = sprite_overlap_check(bullet_sprite, game.bullets[bi].x, game.bullets[bi].y, alien_sprite, alien.x, alien.y);
				if (overlap) {
					game.aliens[ai].type = ALIEN_DEAD;
					game.aliens[ai].x -= (alien_death_sprite.width - alien_sprite.width) / 2; //recenter death animation
					game.bullets[bi] = game.bullets[game.num_bullets - 1];
					game.num_bullets--;
					continue;
				}
			}
			bi++;
		}

		buffer_sprite_draw(&buffer, player_sprite, game.player.x, game.player.y, rgb_to_uint32(128, 0, 0));
		
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer.width, buffer.height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data);
		
		glDrawArrays(GL_TRIANGLES, 0, 3);
		
		glfwSwapBuffers(window); //front buffer is used for displaying an image, while back buffer is used for drawing. They are swapped at each iteration (front displays, back is updated with new actions)
		
		glfwPollEvents(); //poll any new events that might have happened
		
		for (int i = 0; i < 3; i++) {
			alien_animation[i].time++;
			if (alien_animation[i].time == alien_animation[i].num_frames * alien_animation[i].frame_duration) {
				if (alien_animation[i].loop) { //set time back to 0 if animation loops, otherwise delete it
					alien_animation[i].time = 0;
				}
			}
		}

		int player_move_dir = 2 * move_dir;

		if (player_move_dir != 0) {
			if (game.player.x + player_sprite.width + player_move_dir >= game.width - 1) { //keep player within bounds if collision is about to happen
				game.player.x = game.width - player_sprite.width;
			}
			else if ((int)game.player.x + player_move_dir <= 0) {
				game.player.x = 0;
			}
			else game.player.x += player_move_dir;
		}

		for (size_t ai = 0; ai < game.num_aliens; ai++) {
			const Alien& alien = game.aliens[ai];
			if (alien.type == ALIEN_DEAD && death_counters[ai]) {
				death_counters[ai]--; //if alien is dead but its death counter is nonzero, we decrement it
			}
		}

		if (fire_pressed && game.num_bullets < GAME_MAX_BULLETS) {
			game.bullets[game.num_bullets].x = game.player.x + player_sprite.width / 2; //fire from middle of sprite
			game.bullets[game.num_bullets].y = game.player.y + player_sprite.height;
			game.bullets[game.num_bullets].dir = 2;
			game.num_bullets++;
		}
		fire_pressed = false;
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	glDeleteVertexArrays(1, &fullscreen_triangle_vao);

	delete[] alien_sprites[0].data;
	delete[] alien_sprites[1].data;
	delete[] alien_sprites[2].data;
	delete[] alien_sprites[3].data;
	delete[] alien_sprites[4].data;
	delete[] alien_sprites[5].data;

	delete[] player_sprite.data;
	delete[] buffer.data;

	return 0;
}