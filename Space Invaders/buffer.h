#include <cstdio>
#include <cstdint>

struct Buffer { //chunk of memory representing pixels in game
	size_t width, height;
	uint32_t* data;
};

struct Sprite {
	size_t width, height;
	uint8_t* data;
};

struct SpriteAnimation {
	bool loop; //loop over animation or only play it once
	size_t num_frames;
	size_t frame_duration; //time between successive frames
	size_t time; //time in current animation instance
	Sprite** frames;
};

void buffer_clear(Buffer* buffer, uint32_t color);
void buffer_sprite_draw(Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t color);
uint32_t rgb_to_uint32(uint8_t r, uint8_t g, uint8_t b);
bool sprite_overlap_check(const Sprite& sp_a, size_t x_a, size_t y_a, const Sprite& sp_b, size_t x_b, size_t y_b);
void buffer_draw_text(Buffer* buffer, const Sprite& text_spritesheet, const char* text, size_t x, size_t y, uint32_t color);
void buffer_draw_number(Buffer* buffer, const Sprite& number_spritesheet, size_t number, size_t x, size_t y, uint32_t color);
void spriteInit();