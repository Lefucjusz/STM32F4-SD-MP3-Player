/*
 * display.h
 *
 *  Created on: Feb 16, 2023
 *      Author: lefucjusz
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stddef.h>
#include <stdint.h>

/* Custom chars loaded to CGRAM in display_init(); address
 * 0x00 cannot be used because it's string null-terminator
 * in C, so I just picked the next one. */
#define DISPLAY_PAUSE_GLYPH 0x01
#define DISPLAY_PLAY_GLYPH 0x02

/* From HD44780 charset */
#define DISPLAY_BLOCK_GLYPH 0xFF

#define DISPLAY_LINE_NUM 2
#define DISPLAY_LINE_LENGTH 20

void display_init(void);

int display_set_text(const char *text, size_t line_num, uint32_t scroll_delay);
int display_set_text_sync(const char *first_line_text, const char *second_line_text, uint32_t scroll_delay);

void display_task(void);

void display_deinit(void);

#endif /* DISPLAY_H_ */
