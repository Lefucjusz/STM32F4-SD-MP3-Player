/*
 * delay.c
 *
 *  Created on: Mar 9, 2023
 *      Author: lefucjusz
 */

#include "delay.h"

#define DELAY_MSECS_PER_SEC 1000

static TIM_HandleTypeDef *timer = NULL;

void delay_init(TIM_HandleTypeDef *microsecond_timer) {
	timer = microsecond_timer;
}

void delay_us(uint16_t microseconds) {
	/* Sanity check */
	if (timer == NULL) {
		return;
	}

	/* Set timer count to zero and start it */
	__HAL_TIM_SET_COUNTER(timer, 0);
	HAL_TIM_Base_Start(timer);

	/* Wait until in reaches desired value and stop */
	while (__HAL_TIM_GET_COUNTER(timer) < microseconds);
	HAL_TIM_Base_Stop(timer);
}

void delay_ms(uint32_t milliseconds) {
	for (size_t i = 0; i < milliseconds; ++i) {
		delay_us(DELAY_MSECS_PER_SEC);
	}
}
