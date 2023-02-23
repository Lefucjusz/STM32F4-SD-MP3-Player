/*
 * display.c
 *
 *  Created on: Feb 16, 2023
 *      Author: lefucjusz
 */

#include "display.h"

#include "HD44780.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "stm32f4xx_hal.h"

/* Two-space suffix for scrolling to look good */
#define DISPLAY_SUFFIX "  "
#define DISPLAY_SUFFIX_LENGTH 2

/* Pause glyph */
static const uint8_t pause[] = {
	0b00000, 0b01010, 0b01010, 0b01010, 0b01010, 0b01010, 0b00000
};

typedef struct {
	char *line_buffer[DISPLAY_LINE_NUM];
	size_t line_offset[DISPLAY_LINE_NUM];
	uint32_t scroll_delay[DISPLAY_LINE_NUM];
	uint32_t last_refresh_tick[DISPLAY_LINE_NUM];
} display_ctx_t;

static display_ctx_t ctx;

void display_init(void) {
	memset(&ctx, 0, sizeof(display_ctx_t));
	HD44780_load_custom_glyph(pause, DISPLAY_PAUSE_GLYPH);
	HD44780_clear();
}

int display_set_text(const char *text, size_t line_num, uint32_t scroll_delay) {
	if ((line_num < 1) || (line_num > DISPLAY_LINE_NUM)) {
		return -EINVAL;
	}

	free(ctx.line_buffer[line_num - 1]);

	const size_t line_length = strlen(text);
	char *line;

	/* Case without scrolling */
	if (line_length <= DISPLAY_LINE_LENGTH) {
		line = calloc(1, DISPLAY_LINE_LENGTH + 1);
		if (line == NULL) {
			return -ENOMEM;
		}
		memcpy(line, text, line_length);

		const size_t padding = DISPLAY_LINE_LENGTH - line_length;
		memset(&line[line_length], ' ', padding);
		line[line_length + padding] = '\0';
	}
	else {
		line = calloc(1, line_length + DISPLAY_SUFFIX_LENGTH + 1);
		if (line == NULL) {
			return -ENOMEM;
		}
		memcpy(line, text, line_length);
		memcpy(&line[line_length], DISPLAY_SUFFIX, DISPLAY_SUFFIX_LENGTH + 1);
	}

	ctx.line_buffer[line_num - 1] = line;
	ctx.scroll_delay[line_num - 1] = scroll_delay;
	ctx.line_offset[line_num - 1] = 0;
	ctx.last_refresh_tick[line_num - 1] = 0;

	return 0;
}

int display_set_text_sync(const char *first_line_text, const char *second_line_text, uint32_t scroll_delay) {
	const size_t first_length = strlen(first_line_text);
	const size_t second_length = strlen(second_line_text);

	/* Fallback to regular text setting so that the line that fits is not scrolled */
	if ((first_length <= DISPLAY_LINE_LENGTH) || (second_length <= DISPLAY_LINE_LENGTH)) {
		int ret = display_set_text(first_line_text, 1, scroll_delay);
		if (ret) {
			return ret;
		}

		ret = display_set_text(second_line_text, 2, scroll_delay);
		return ret;
	}

	free(ctx.line_buffer[0]);
	free(ctx.line_buffer[1]);

	const size_t max_length = (first_length > second_length) ? first_length : second_length;

	char *first_line = calloc(1, max_length + DISPLAY_SUFFIX_LENGTH + 1);
	if (first_line == NULL) {
		return -ENOMEM;
	}

	char *second_line = calloc(1, max_length + DISPLAY_SUFFIX_LENGTH + 1);
	if (second_line == NULL) {
		free(first_line);
		return -ENOMEM;
	}

	memcpy(first_line, first_line_text, first_length);
	memcpy(second_line, second_line_text, second_length);

	const size_t padding = abs((long)first_length - (long)second_length);
	if (first_length < second_length) {
		memset(&first_line[first_length], ' ', padding);
	}
	else {
		memset(&second_line[second_length], ' ', padding);
	}

	memcpy(&first_line[max_length], DISPLAY_SUFFIX, DISPLAY_SUFFIX_LENGTH + 1);
	memcpy(&second_line[max_length], DISPLAY_SUFFIX, DISPLAY_SUFFIX_LENGTH + 1);

	ctx.line_buffer[0] = first_line;
	ctx.scroll_delay[0] = scroll_delay;
	ctx.line_offset[0] = 0;
	ctx.last_refresh_tick[0] = 0;

	ctx.line_buffer[1] = second_line;
	ctx.scroll_delay[1] = scroll_delay;
	ctx.line_offset[1] = 0;
	ctx.last_refresh_tick[1] = 0;

	return 0;
}

void display_task(void) {
	uint32_t current_tick = HAL_GetTick();

	for (size_t i = 0; i < DISPLAY_LINE_NUM; ++i) {
		/* Skip if no line buffer or line empty */
		if ((ctx.line_buffer[i] == NULL) || (ctx.line_buffer[i][0] == '\0')) {
			continue;
		}


		if ((current_tick - ctx.last_refresh_tick[i]) >= ctx.scroll_delay[i]) {
			const size_t text_line_length = strlen(ctx.line_buffer[i]);

			/* Display statically if it fits */
			if (text_line_length <= DISPLAY_LINE_LENGTH) {
				if (ctx.line_offset[i] > 0) {
					continue;
				}
				HD44780_gotoxy(i + 1, 1);
				HD44780_write_string(ctx.line_buffer[i]);
				ctx.line_offset[i]++;
			}
			else {
				/* Scroll if not */
				char line_buffer[DISPLAY_LINE_LENGTH + 1];

				for (size_t column = 0; column < DISPLAY_LINE_LENGTH; ++column) {
					const size_t src_column = (ctx.line_offset[i] + column) % text_line_length;
					line_buffer[column] = ctx.line_buffer[i][src_column];
				}

				HD44780_gotoxy(i + 1, 1);
				HD44780_write_string(line_buffer);

				ctx.line_offset[i]++;
				ctx.last_refresh_tick[i] = current_tick;
			}
		}
	}
}

void display_cleanup(void) {
	for (size_t i = 0; i < DISPLAY_LINE_NUM; ++i) {
		free(ctx.line_buffer[i]);
	}
}
