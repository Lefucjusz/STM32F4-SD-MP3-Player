/*
 * HD44780.c
 *
 *  Created on: 16.05.2022
 *      Author: Lefucjusz
 */

#include "HD44780.h"
#include <string.h>
#include <stdlib.h>

/* Needed in HD44780_write_integer() */
#define HD44780_TMP_BUF_SIZE 11 // Value stored in int32_t has at most 10 digits - 2147483647 - one additional byte for null-terminator
#define HD44780_CGRAM_CHAR_SIZE 8 // Each custom char uses 8 bytes

typedef struct {
	size_t rows;
	size_t columns;
	uint8_t rows_first_addr[4]; // Display has at most 4 rows
} HD44780_type_data_t;

static HD44780_type_data_t HD44780_type_data[HD44780_DISPLAY_TYPES_NUM] = {
		/* This is super weird display, it's 16x1, but behaves like 8x2 */
		{.rows = 2, .columns = 8, .rows_first_addr[0] = 0x00, .rows_first_addr[1] = 0x40},
		{.rows = 1, .columns = 16, .rows_first_addr[0] = 0x00},
		{.rows = 2, .columns = 16, .rows_first_addr[0] = 0x00, .rows_first_addr[1] = 0x40},
		{.rows = 4, .columns = 16, .rows_first_addr[0] = 0x00, .rows_first_addr[1] = 0x40, .rows_first_addr[2] = 0x10, .rows_first_addr[3] = 0x50},
		{.rows = 2, .columns = 20, .rows_first_addr[0] = 0x00, .rows_first_addr[1] = 0x40},
		{.rows = 4, .columns = 20, .rows_first_addr[0] = 0x00, .rows_first_addr[1] = 0x40, .rows_first_addr[2] = 0x14, .rows_first_addr[3] = 0x54},
		{.rows = 2, .columns = 40, .rows_first_addr[0] = 0x00, .rows_first_addr[1] = 0x40},
};

static HD44780_config_t* HD44780_config = NULL;

void HD44780_init(HD44780_config_t* const config) {
	HD44780_config = config;

	/* If type is invalid, choose the most popular display */
	if ((HD44780_config->type < 0) || (HD44780_config->type >= HD44780_DISPLAY_TYPES_NUM)) {
		HD44780_config->type = HD44780_DISPLAY_16x2;
	}

	HD44780_write_cmd(0x03);
	HD44780_config->io->delay_us(4500); // Wait for more than 4.1ms (HD44780 datasheet, Fig. 24, p. 46)
	HD44780_write_cmd(0x03);
	HD44780_config->io->delay_us(150); // Wait for more than 100us
	HD44780_write_cmd(0x03);
	HD44780_config->io->delay_us(100); // Not specified in DS, chosen empirically
	HD44780_write_cmd(0x02);

	/* Here begins the real configuration */
	HD44780_type_data_t type_data = HD44780_type_data[HD44780_config->type];
	if (type_data.rows == 1) {
		HD44780_write_cmd(HD44780_FUNCTION_SET_CMD | HD44780_ONE_LINE); // Initialize as 1 line, 5x8 matrix, 4-bit interface
	}
	else {
		HD44780_write_cmd(HD44780_FUNCTION_SET_CMD | HD44780_TWO_LINES); // Initialize as 2 lines, 5x8 matrix, 4-bit interface
	}

	HD44780_write_cmd(HD44780_DISPLAY_ON_OFF_CMD | HD44780_config->on_off_flags); // Set display on/off flags
	HD44780_write_cmd(HD44780_ENTRY_MODE_SET_CMD | HD44780_config->entry_mode_flags); // Set entry mode flags

	HD44780_clear();
}

void HD44780_write_byte(uint8_t byte, HD44780_mode_t mode) {
	/* Set RS line state */
	HD44780_config->io->set_pin_state(HD44780_PIN_RS, (mode == HD44780_CHARACTER) ? HD44780_HIGH : HD44780_LOW);

	/* Write upper nibble */
	HD44780_config->io->set_pin_state(HD44780_PIN_D7, (byte & (1 << 7)) ? HD44780_HIGH : HD44780_LOW);
	HD44780_config->io->set_pin_state(HD44780_PIN_D6, (byte & (1 << 6)) ? HD44780_HIGH : HD44780_LOW);
	HD44780_config->io->set_pin_state(HD44780_PIN_D5, (byte & (1 << 5)) ? HD44780_HIGH : HD44780_LOW);
	HD44780_config->io->set_pin_state(HD44780_PIN_D4, (byte & (1 << 4)) ? HD44780_HIGH : HD44780_LOW);

	/* Pulse enable signal */
	HD44780_config->io->set_pin_state(HD44780_PIN_E, HD44780_HIGH);
	HD44780_config->io->delay_us(1); // At least 450ns (HD44780 datasheet, p. 49)
	HD44780_config->io->set_pin_state(HD44780_PIN_E, HD44780_LOW);

	/* Write lower nibble */
	HD44780_config->io->set_pin_state(HD44780_PIN_D7, (byte & (1 << 3)) ? HD44780_HIGH : HD44780_LOW);
	HD44780_config->io->set_pin_state(HD44780_PIN_D6, (byte & (1 << 2)) ? HD44780_HIGH : HD44780_LOW);
	HD44780_config->io->set_pin_state(HD44780_PIN_D5, (byte & (1 << 1)) ? HD44780_HIGH : HD44780_LOW);
	HD44780_config->io->set_pin_state(HD44780_PIN_D4, (byte & (1 << 0)) ? HD44780_HIGH : HD44780_LOW);

	/* Pulse enable signal */
	HD44780_config->io->set_pin_state(HD44780_PIN_E, HD44780_HIGH);
	HD44780_config->io->delay_us(1);
	HD44780_config->io->set_pin_state(HD44780_PIN_E, HD44780_LOW);

	/* Wait at least 37us for command to be executed (HD44780 datasheet, Table 6, p. 24) */
	HD44780_config->io->delay_us(50);
}

void HD44780_write_cmd(uint8_t command) {
	HD44780_write_byte(command, HD44780_INSTRUCTION);
}

void HD44780_write_char(char character) {
	HD44780_write_byte(character, HD44780_CHARACTER);
}

void HD44780_clear(void) {
	/* Clear display */
	HD44780_write_cmd(HD44780_CLEAR_DISPLAY_CMD);
	HD44780_config->io->delay_us(1600); // At least 1.52ms (HD44780 datasheet, Table 6, p. 24)

	/* Set cursor to the first column of the first row */
	HD44780_type_data_t type_data = HD44780_type_data[HD44780_config->type];
	HD44780_write_cmd(HD44780_SET_DDRAM_ADDR_CMD | type_data.rows_first_addr[0]);
}

void HD44780_gotoxy(size_t x, size_t y) {
	/* Get data for selected display type */
	HD44780_type_data_t type_data = HD44780_type_data[HD44780_config->type];

	/* If rows < 1, select the first one */
	if (x < 1) {
		x = 1;
	}
	/* If rows out of range, select the last one */
	if (x > type_data.rows) {
		x = type_data.rows;
	}

	/* If columns < 1, select the first one */
	if (y < 1) {
		y = 1;
	}
	/* If columns out of range, select the last one */
	if (y > type_data.columns) {
		y = type_data.columns;
	}

	/* Get address of selected row, add column offset */
	uint8_t required_address = type_data.rows_first_addr[x - 1] + y - 1;

	/* Move to requested position */
	HD44780_write_cmd(HD44780_SET_DDRAM_ADDR_CMD | required_address);
}

void HD44780_write_integer(int32_t number, size_t required_length) {
	/* If number is negative, display minus sign and convert it to positive */
	if (number < 0) {
		HD44780_write_char('-');
		number = -number;
	}

	/* Convert number to string */
	char buffer[HD44780_TMP_BUF_SIZE];
	itoa(number, buffer, 10);

	/* Leading zeros handling - compute how many should be appended to get the required length and display them */
	int leading_zeros = required_length - strlen(buffer);
	while (leading_zeros > 0) {
		HD44780_write_char('0');
		leading_zeros--;
	}

	HD44780_write_string(buffer);
}

void HD44780_write_string(const char* string) {
	while (*string != '\0') {
		HD44780_write_char(*string++);
	}
}

void HD44780_load_custom_glyph(const uint8_t* const glyph_array, HD44780_glyph_addr_t cgram_addr) {
	/* If provided address out of range, select last one */
	if (cgram_addr > HD44780_CUSTOM_GLYPH_7) {
		cgram_addr = HD44780_CUSTOM_GLYPH_7;
	}

	/* Set CGRAM pointer to required location */
	size_t cgram_offset = cgram_addr * HD44780_CGRAM_CHAR_SIZE;
	HD44780_write_cmd(HD44780_SET_CGRAM_ADDR_CMD | cgram_offset);

	/* Load character */
	for (size_t i = 0; i < HD44780_CGRAM_CHAR_SIZE; i++) {
		HD44780_write_char(glyph_array[i]);
	}
}

void HD44780_load_custom_glyphs(const uint8_t* const glyphs_array) {
	for (HD44780_glyph_addr_t i = HD44780_CUSTOM_GLYPH_0; i < HD44780_CUSTOM_GLYPHS_NUM; i++) {
		HD44780_load_custom_glyph(&glyphs_array[i * HD44780_CGRAM_CHAR_SIZE], i);
	}
}
