/*
 * keys.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 27 Jan 2021
 *      Author: mario
 *
 *  This software and all related data files can be used within the terms of GNU GPLv3 license:
 *  https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *
 *  http://phonicbloom.com/
 *  http://gechologic.com/
 *  http://loopstyler.com/
 *  http://doniguano.com/
 *
 */

#ifndef KEYS_H_
#define KEYS_H_

#define KEY_SET_HELD		10
#define KEY_SHIFT_HELD		11
#define KEY_BOTH_HELD		12
#define KEY_SHIFT_SET		13
#define KEY_SET_CLICKED		14
#define KEY_SHIFT_CLICKED	15

#define KEY_SHIFT			9
#define KEY_SET				0
#define IS_KEYBOARD(k)		(k>KEY_SET && k<KEY_SHIFT)
#define IS_KEY_SHIFT(k)		(k==KEY_SHIFT)
#define IS_KEY_SET(k)		(k==KEY_SET)

//these 4 definitions allows to swap SET and SHIFT key
#define IS_KEY_MENU(k)		(k==KEY_SHIFT) //change accordingly with two definitions below
#define IS_KEY_ASSIGN(k)	(k==KEY_SET)
#define MENU_KEY_HELD		shift_pressed //can be set_pressed or shift_pressed
#define ASSIGN_KEY_HELD		set_pressed //can be set_pressed or shift_pressed

#define IS_KEY_EXIT(k)		(k==KEY_SHIFT || k==KEY_SET) //change according to which key should exit the menu

#define EXIT_CLICKED(k)		(k==KEY_SHIFT_CLICKED || k==KEY_SET_CLICKED)
#define SHIFT_CLICKED(k)	(k==KEY_SHIFT_CLICKED)
#define SET_CLICKED(k)		(k==KEY_SET_CLICKED)

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported functions ------------------------------------------------------- */

void trigger_patch_octave(int k, int octave_div);
void trigger_patch(int k);
void scan_keypad();

#ifdef __cplusplus
}
#endif

#endif /* KEYS_H_ */
