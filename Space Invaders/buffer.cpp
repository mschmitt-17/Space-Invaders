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

bool sprite_overlap_check(const Sprite& sp_a, size_t x_a, size_t y_a, const Sprite& sp_b, size_t x_b, size_t y_b) {
	if (x_a < x_b + sp_b.width && x_a + sp_a.width > x_b && y_a < y_b + sp_b.height && y_a + sp_a.height > y_b) {
		return true;
	}
	return false;
}

void buffer_draw_text(Buffer* buffer, const Sprite& text_spritesheet, const char* text, size_t x, size_t y, uint32_t color) {
	size_t xp = x;
	size_t stride = text_spritesheet.width * text_spritesheet.height; //size of one character (35 in this case)
	Sprite sprite = text_spritesheet;
	for (const char* charp = text; *charp != '\0'; charp++) {
		char character = *charp - 32;
		if (character < 0 || character >= 65) continue; //if ascii code is less than 0 or greater than 35 it isn't in our spritesheet

		sprite.data = text_spritesheet.data + character * stride; //character position given by ascii code of character - 32 times stride (like 120 lab)
		buffer_sprite_draw(buffer, sprite, xp, y, color);
		xp += sprite.width + 1; //character drawing position moved by sprite width so we can print the next character without overlap
	}
}

void buffer_draw_number(Buffer* buffer, const Sprite& number_spritesheet, size_t number, size_t x, size_t y, uint32_t color) { //figure this out
	uint8_t digits[64];
	size_t num_digits = 0;
	size_t current_number = number;
	do {
		digits[num_digits++] = current_number % 10; //less significant digits in lower index positions in digits (i.e. full number is backwards in digits)
		current_number = current_number / 10;
	} while (current_number > 0);

	size_t xp = x;
	size_t stride = number_spritesheet.width * number_spritesheet.height;
	Sprite sprite = number_spritesheet;
	for (size_t i = 0; i < num_digits; i++) {
		uint8_t digit = digits[num_digits - i - 1]; //going to lower indexes (less significant numbers) in digits as i progresses
		sprite.data = number_spritesheet.data + digit * stride;
		buffer_sprite_draw(buffer, sprite, xp, y, color);
		xp += sprite.width + 1;
	}
}

void spriteInit() {
	
}
