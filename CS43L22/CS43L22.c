#include "CS43L22.h"
#include "delay.h"

#define CS43L22_ADDRESS_SIZE 1 // byte

bool CS43L22_power_up(I2C_HandleTypeDef *i2c) {
	HAL_StatusTypeDef i2c_status;
	uint8_t reg_value;

	/* Power up the chip */
	reg_value = 0b10011110;
	i2c_status = HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_POWER_CTL_1_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	return ((i2c_status == HAL_OK) ? true : false);
}

bool CS43L22_power_down(I2C_HandleTypeDef *i2c) {
	HAL_StatusTypeDef i2c_status;
	uint8_t reg_value;

	/* Power down the chip */
	reg_value = 0b10011111;
	i2c_status = HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_POWER_CTL_1_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);
	delay_ms(1); // Datasheet specifies "at least 100us" (section 4.10, p. 31)

	return ((i2c_status == HAL_OK) ? true : false);
}

bool CS43L22_init(I2C_HandleTypeDef *i2c) {
	HAL_StatusTypeDef i2c_status;
	bool status;
	uint8_t reg_value;

	/* Release reset state */
	HAL_GPIO_WritePin(CS43L22_RESET_GPIO, CS43L22_RESET_PIN, GPIO_PIN_SET);

	/* Check if chip present */
	i2c_status = HAL_I2C_Mem_Read(i2c, CS43L22_I2C_ADDRESS, CS43L22_ID_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);
	if ((i2c_status != HAL_OK) || ((reg_value & CS43L22_ID_MASK) != CS43L22_ID_VALUE)) {
		return false;
	}

	/* Power down the chip */
	status = CS43L22_power_down(i2c);

	/* Magic procedure from CS43L22's datasheet, section 4.11, p.32 */
	reg_value = 0x99;
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, 0x00, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	reg_value = 0x80;
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, 0x47, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	i2c_status |= HAL_I2C_Mem_Read(i2c, CS43L22_I2C_ADDRESS, 0x32, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);
	reg_value |= (1 << 7);
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, 0x32, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	i2c_status |= HAL_I2C_Mem_Read(i2c, CS43L22_I2C_ADDRESS, 0x32, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);
	reg_value &= ~(1 << 7);
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, 0x32, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	reg_value = 0x00;
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, 0x00, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	/* Speaker channels always off, headphones channels always on */
	reg_value = 0b10101111;
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_POWER_CTL_2_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	/* Enable autodetection of the clock speed */
	reg_value = 0b10000000;
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_CLOCKING_CTL_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	/* Enable 16-bit Philips I2S format */
	reg_value = 0b00000111;
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_INTERFACE_CTL_1_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);


	/* The configurations below are done to reduce the time needed for CS43L22 to power off.
	 * If these configurations are removed, then a long delay should be added between
	 * powering off the device and switching off the I2S peripheral MCLK clock. If this
	 * delay is not inserted, then the codec will not shut down properly, what results
	 * in high noise after shut down. */

	/* Disable analog soft ramp */
	reg_value = 0b00000000;
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_ANALOG_ZC_SR_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	/* Disable digital soft ramp */
	reg_value = 0b00000000;
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_MISC_CTL_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	/* Turn on the chip */
	status &= CS43L22_power_up(i2c);

	status &= (i2c_status == HAL_OK);
	return status;
}

bool CS43L22_set_volume(I2C_HandleTypeDef *i2c, int8_t volume) {
	HAL_StatusTypeDef i2c_status;
	uint8_t reg_value;

	/* Read register to preserve current mute value */
	i2c_status = HAL_I2C_Mem_Read(i2c, CS43L22_I2C_ADDRESS, CS43L22_PCMA_VOL_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	/* Clear volume bits and set new volume value, masking MSB */
	reg_value &= CS43L22_PCM_MUTE_MASK;
	reg_value |= (volume & ~CS43L22_PCM_MUTE_MASK);

	/* Write back the value */
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_PCMA_VOL_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	/* Repeat for the second channel */
	i2c_status |= HAL_I2C_Mem_Read(i2c, CS43L22_I2C_ADDRESS, CS43L22_PCMB_VOL_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	reg_value &= CS43L22_PCM_MUTE_MASK;
	reg_value |= (volume & ~CS43L22_PCM_MUTE_MASK);

	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_PCMB_VOL_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	return ((i2c_status == HAL_OK) ? true : false);
}

bool CS43L22_mute_pcm(I2C_HandleTypeDef *i2c, bool mute) {
	HAL_StatusTypeDef i2c_status;
	uint8_t reg_value;

	/* Read register to preserve current status of the other bits */
	i2c_status = HAL_I2C_Mem_Read(i2c, CS43L22_I2C_ADDRESS, CS43L22_PCMA_VOL_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	/* Set mute bit */
	if (mute) {
		reg_value |= CS43L22_PCM_MUTE_MASK;
	}
	else {
		reg_value &= ~CS43L22_PCM_MUTE_MASK;
	}

	/* Write back the value */
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_PCMA_VOL_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	/* Repeat for the second channel */
	i2c_status |= HAL_I2C_Mem_Read(i2c, CS43L22_I2C_ADDRESS, CS43L22_PCMB_VOL_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	if (mute) {
		reg_value |= CS43L22_PCM_MUTE_MASK;
	}
	else {
		reg_value &= ~CS43L22_PCM_MUTE_MASK;
	}

	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_PCMB_VOL_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	return ((i2c_status == HAL_OK) ? true : false);
}

bool CS43L22_mute_headphones(I2C_HandleTypeDef *i2c, bool mute) {
	HAL_StatusTypeDef i2c_status;
	uint8_t reg_value;

	/* Read register to preserve current volume value */
	i2c_status = HAL_I2C_Mem_Read(i2c, CS43L22_I2C_ADDRESS, CS43L22_PLAYBACK_CTL_2_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	/* Set mute bit */
	if (mute) {
		reg_value |= CS43L22_HEADPHONES_MUTE_MASK;
	}
	else {
		reg_value &= ~CS43L22_HEADPHONES_MUTE_MASK;
	}

	/* Write back the value */
	i2c_status |= HAL_I2C_Mem_Write(i2c, CS43L22_I2C_ADDRESS, CS43L22_PLAYBACK_CTL_2_REG, CS43L22_ADDRESS_SIZE, &reg_value, sizeof(reg_value), CS43L22_I2C_TIMEOUT);

	return ((i2c_status == HAL_OK) ? true : false);
}

bool CS43L22_mute(I2C_HandleTypeDef *i2c, bool mute) {
	bool status;

	status = CS43L22_mute_headphones(i2c, mute);
	status &= CS43L22_mute_pcm(i2c, mute);

	return status;
}

bool CS43L22_deinit(I2C_HandleTypeDef *i2c) {
	bool status;

	status = CS43L22_mute(i2c, true);
	status &= CS43L22_power_down(i2c);

	/* Bring device to reset state */
	HAL_GPIO_WritePin(CS43L22_RESET_GPIO, CS43L22_RESET_PIN, GPIO_PIN_RESET);

	return status;
}
