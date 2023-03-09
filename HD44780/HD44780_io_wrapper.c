/*
 * HD44780_io.c
 *
 *  Created on: Feb 15, 2023
 *      Author: lefucjusz
 */

#include "HD44780_io_wrapper.h"
#include "stm32f4xx_hal.h"
#include "delay.h"

#define HD44780_GPIO_PORT GPIOD

typedef struct {
	uint16_t gpio_pin;
	HD44780_pin_t display_pin;
} HD44780_gpio_map_t;

static HD44780_gpio_map_t gpio_map[HD44780_PIN_NUM] = {
		{.gpio_pin = GPIO_PIN_0, .display_pin = HD44780_PIN_D4},
		{.gpio_pin = GPIO_PIN_1, .display_pin = HD44780_PIN_D5},
		{.gpio_pin = GPIO_PIN_2, .display_pin = HD44780_PIN_D6},
		{.gpio_pin = GPIO_PIN_3, .display_pin = HD44780_PIN_D7},
		{.gpio_pin = GPIO_PIN_6, .display_pin = HD44780_PIN_RS},
		{.gpio_pin = GPIO_PIN_7, .display_pin = HD44780_PIN_E}
};

static void set_pin_state(HD44780_pin_t pin, HD44780_pin_state_t state) {
	for (size_t i = 0; i < HD44780_PIN_NUM; ++i) {
		if (gpio_map[i].display_pin == pin) {
			HAL_GPIO_WritePin(HD44780_GPIO_PORT, gpio_map[i].gpio_pin, state);
			break;
		}
	}
}

HD44780_io_t *HD44780_get_io(void) {
	static HD44780_io_t io = {
			.set_pin_state = set_pin_state,
			.delay_us = delay_us
	};

	return &io;
}
