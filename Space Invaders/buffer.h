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

void buffer_clear(Buffer* buffer, uint32_t color);
void buffer_sprite_draw(Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t color);
uint32_t rgb_to_uint32(uint8_t r, uint8_t g, uint8_t b);