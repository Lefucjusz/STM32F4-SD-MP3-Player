/*
 * player.h
 *
 *  Created on: Feb 19, 2023
 *      Author: lefucjusz
 */

#ifndef PLAYER_H_
#define PLAYER_H_

#include <stdbool.h>
#include "stm32f4xx_hal.h"

#define PLAYER_BUFFER_SIZE_SAMPLES 16384
#define PLAYER_CHANNELS_NUM 2

typedef enum {
	PLAYER_STOPPED,
	PLAYER_PAUSED,
	PLAYER_PLAYING
} player_state_t;

void player_init(I2S_HandleTypeDef *i2s, I2C_HandleTypeDef *i2c);

int player_start(const char *path);
void player_pause(void);
void player_resume(void);
void player_stop(void);

bool player_set_volume(int8_t volume);

player_state_t player_get_state(void);
uint64_t player_get_frames_played(void);
uint32_t player_get_pcm_sample_rate(void);
uint32_t player_get_mp3_frame_bitrate(void);

void player_task(void);

#endif /* PLAYER_H_ */
