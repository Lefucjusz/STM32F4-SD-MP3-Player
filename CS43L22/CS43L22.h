#pragma once

#include <stdbool.h>
#include "stm32f4xx_hal.h"

/* Config */
#define CS43L22_RESET_GPIO GPIOD
#define CS43L22_RESET_PIN GPIO_PIN_4
#define CS43L22_I2C_TIMEOUT 100 // ms

/* I2C address */
#define CS43L22_I2C_ADDRESS 0b10010100

/* Chip's ID value */
#define CS43L22_ID_VALUE 0b11100000

/* Volume limits, one step is 0.5dB change */
#define CS43L22_VOLUME_STEPS_PER_DB 2
#define CS43L22_MAX_VOLUME (12 * CS43L22_VOLUME_STEPS_PER_DB) // +12dB
#define CS43L22_MIN_VOLUME (-51.5 * CS43L22_VOLUME_STEPS_PER_DB) // -51.5dB

/* Masks */
#define CS43L22_ID_MASK 0b11111000 // To mask chip revision bits
#define CS43L22_PCM_MUTE_MASK (1 << 7)
#define CS43L22_HEADPHONES_MUTE_MASK ((1 << 7) | (1 << 6))

/* Registers */
#define CS43L22_ID_REG 0x01
#define CS43L22_POWER_CTL_1_REG 0x02
#define CS43L22_POWER_CTL_2_REG 0x04
#define CS43L22_CLOCKING_CTL_REG 0x05
#define CS43L22_INTERFACE_CTL_1_REG 0x06
#define CS43L22_INTERFACE_CTL_2_REG 0x07
#define CS43L22_PASSTHRU_A_SEL_REG 0x08
#define CS43L22_PASSTHRU_B_SEL_REG 0x09
#define CS43L22_ANALOG_ZC_SR_REG 0x0A
#define CS43L22_MISC_CTL_REG 0x0E
#define CS43L22_PLAYBACK_CTL_2_REG 0x0F
#define CS43L22_PCMA_VOL_REG 0x1A
#define CS43L22_PCMB_VOL_REG 0x1B

/* Functions */
bool CS43L22_power_up(I2C_HandleTypeDef *i2c);
bool CS43L22_power_down(I2C_HandleTypeDef *i2c);

bool CS43L22_init(I2C_HandleTypeDef *i2c);

bool CS43L22_set_volume(I2C_HandleTypeDef *i2c, int8_t volume);

bool CS43L22_mute_pcm(I2C_HandleTypeDef *i2c, bool mute);
bool CS43L22_mute_headphones(I2C_HandleTypeDef *i2c, bool mute);
bool CS43L22_mute(I2C_HandleTypeDef *i2c, bool mute);

bool CS43L22_deinit(I2C_HandleTypeDef *i2c);
