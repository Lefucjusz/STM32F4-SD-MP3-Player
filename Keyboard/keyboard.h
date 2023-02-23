/*
 * keyboard.h
 *
 *  Created on: Feb 18, 2023
 *      Author: lefucjusz
 */

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

typedef enum {
	KEYBOARD_UP,
	KEYBOARD_DOWN,
	KEYBOARD_LEFT,
	KEYBOARD_RIGHT,
	KEYBOARD_ENTER,
	KEYBOARD_BUTTONS_NUM
} keyboard_buttons_t;

void keyboard_init(void);

void keyboard_attach_callback(keyboard_buttons_t button, void (*callback)(void));

void keyboard_task(void);

#endif /* KEYBOARD_H_ */
