/*
 * delay.h
 *
 *  Created on: Mar 9, 2023
 *      Author: lefucjusz
 */

#ifndef DELAY_H_
#define DELAY_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

void delay_init(TIM_HandleTypeDef *microsecond_timer);
void delay_us(uint16_t microseconds);
void delay_ms(uint32_t milliseconds);

#endif /* DELAY_H_ */
