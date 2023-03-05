/*
 * gui.h
 *
 *  Created on: Feb 18, 2023
 *      Author: lefucjusz
 */

#ifndef GUI_H_
#define GUI_H_

#include "CS43L22.h"

#define GUI_SCROLL_DELAY 250 // ms

#define GUI_MAX_VOLUME CS43L22_MAX_VOLUME
#define GUI_MIN_VOLUME CS43L22_MIN_VOLUME
#define GUI_DEFAULT_VOLUME (-39 * CS43L22_VOLUME_STEPS_PER_DB) // -39dB
#define GUI_VOLUME_STEP (3 * CS43L22_VOLUME_STEPS_PER_DB) // 3dB


void gui_init(void);

void gui_task(void);

void gui_deinit(void);

#endif /* GUI_H_ */
