#include "buffer.h"

void buffer_clear(Buffer* buffer, uint32_t color) {
	for (size_t i = 0; i < buffer->width * buffer->height; i++) {
		buffer->data[i] = color; //iterates over all pixels, setting pixels to given color
	}
}

void buffer_sprite_draw(Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t color) { //draws pixels at specified coordinate for sprite
	for (size_t xi = 0; xi < sprite.width; xi++) {
		for (size_t yi = 0; yi < sprite.height; yi++) {
			size_t sy = sprite.height - 1 + y - yi;
			size_t sx = x + xi;
			if (sprite.data[yi * sprite.width + xi] && sy < buffer->height && sx < buffer->width) {
				buffer->data[sy * buffer->width + sx] = color;
			}
		}
	}
}

uint32_t rgb_to_uint32(uint8_t r, uint8_t g, uint8_t b) { //each pixil is represented as a 32 bit value, with 8 bits for each r, g, and b
	return (r << 24) | (g << 16) | (b << 8) | 255; //left shifts r, g, b into 24 MSBs, sets 8 LSBs to 1
}
