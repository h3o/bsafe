/*
 * gpio.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 30 May 2020
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

#ifndef GPIO_H_
#define GPIO_H_

#include <stdbool.h>
#include "driver/gpio.h"
#include "board.h"

#define BUTTON_U1_PIN		GPIO_NUM_35 //PWR button
#define BUTTON_U2_PIN		GPIO_NUM_36 //BOOT button - metal
#define BUTTON_U3_PIN		GPIO_NUM_39 //RST button - wood
#define BUTTON_U4_PIN		GPIO_NUM_25	//minus
#define BUTTON_U5_PIN		GPIO_NUM_26	//plus

#define LEDS_ALL_OFF		{gpio_set_level(LED_A0,0);gpio_set_level(LED_A1,0);gpio_set_level(LED_A2,0);gpio_set_level(LED_A3,0);gpio_set_level(LED_C0,1);gpio_set_level(LED_C1,1);gpio_set_level(LED_C2,1);gpio_set_level(LED_C3,1);}

extern uint8_t LED_map[4];

#define LED_2_ON	{gpio_set_level(LED_A2,1);gpio_set_level(LED_C0,0);}
#define LED_2_OFF	{gpio_set_level(LED_A2,0);gpio_set_level(LED_C0,1);}
#define LED_0_ON	{gpio_set_level(LED_A2,1);gpio_set_level(LED_C1,0);}
#define LED_0_OFF	{gpio_set_level(LED_A2,0);gpio_set_level(LED_C1,1);}
#define LED_1_ON	{gpio_set_level(LED_A2,1);gpio_set_level(LED_C2,0);}
#define LED_1_OFF	{gpio_set_level(LED_A2,0);gpio_set_level(LED_C2,1);}
#define LED_3_ON	{gpio_set_level(LED_A2,1);gpio_set_level(LED_C3,0);}
#define LED_3_OFF	{gpio_set_level(LED_A2,0);gpio_set_level(LED_C3,1);}

#define LED_6_ON	{gpio_set_level(LED_A0,1);gpio_set_level(LED_C0,0);}
#define LED_6_OFF	{gpio_set_level(LED_A0,0);gpio_set_level(LED_C0,1);}
#define LED_4_ON	{gpio_set_level(LED_A0,1);gpio_set_level(LED_C1,0);}
#define LED_4_OFF	{gpio_set_level(LED_A0,0);gpio_set_level(LED_C1,1);}
#define LED_5_ON	{gpio_set_level(LED_A0,1);gpio_set_level(LED_C2,0);}
#define LED_5_OFF	{gpio_set_level(LED_A0,0);gpio_set_level(LED_C2,1);}
#define LED_7_ON	{gpio_set_level(LED_A0,1);gpio_set_level(LED_C3,0);}
#define LED_7_OFF	{gpio_set_level(LED_A0,0);gpio_set_level(LED_C3,1);}

#define LED_10_ON	{gpio_set_level(LED_A3,1);gpio_set_level(LED_C0,0);}
#define LED_10_OFF	{gpio_set_level(LED_A3,0);gpio_set_level(LED_C0,1);}
#define LED_8_ON	{gpio_set_level(LED_A3,1);gpio_set_level(LED_C1,0);}
#define LED_8_OFF	{gpio_set_level(LED_A3,0);gpio_set_level(LED_C1,1);}
#define LED_9_ON	{gpio_set_level(LED_A3,1);gpio_set_level(LED_C2,0);}
#define LED_9_OFF	{gpio_set_level(LED_A3,0);gpio_set_level(LED_C2,1);}
#define LED_11_ON	{gpio_set_level(LED_A3,1);gpio_set_level(LED_C3,0);}
#define LED_11_OFF	{gpio_set_level(LED_A3,0);gpio_set_level(LED_C3,1);}

#define LED_14_ON	{gpio_set_level(LED_A1,1);gpio_set_level(LED_C0,0);}
#define LED_14_OFF	{gpio_set_level(LED_A1,0);gpio_set_level(LED_C0,1);}
#define LED_12_ON	{gpio_set_level(LED_A1,1);gpio_set_level(LED_C1,0);}
#define LED_12_OFF	{gpio_set_level(LED_A1,0);gpio_set_level(LED_C1,1);}
#define LED_13_ON	{gpio_set_level(LED_A1,1);gpio_set_level(LED_C2,0);}
#define LED_13_OFF	{gpio_set_level(LED_A1,0);gpio_set_level(LED_C2,1);}
#define LED_15_ON	{gpio_set_level(LED_A1,1);gpio_set_level(LED_C3,0);}
#define LED_15_OFF	{gpio_set_level(LED_A1,0);gpio_set_level(LED_C3,1);}

#define LED_TEST_DELAY 	120*4

#define ERROR_BLINK_PATTERN_1357_2468	1
#define ERROR_BLINK_PATTERN_1256_3478	2

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported functions ------------------------------------------------------- */

void Delay(int d);
void usDelay(uint32_t d);

void error_blink(int pattern, int delay);
void set_led(int led, int state);

//void divide_sequencer_steps(int steps, int8_t *r1, int8_t *r2);
void indicate_sequencer_steps(int row1, int row2);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H_ */
