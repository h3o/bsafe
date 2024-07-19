/*
 * gpio.c
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

#include <init.h>

#include <hw/gpio.h>
#include <string.h>

uint8_t LED_map[4] = {0x55,0x55,0x55,0x55};	/* two bits per LED:
												00 = output is set LOW (LED on)
												01 = output is set high-impedance (LED off; default)
												10 = output blinks at PWM0 rate
												11 = output blinks at PWM1 rate */
void Delay(int d)
{
	vTaskDelay(d / portTICK_RATE_MS);
}

void usDelay(uint32_t d)
{
	ets_delay_us(d);
}

void error_blink(int pattern, int delay)
{
	if(pattern == ERROR_BLINK_PATTERN_1357_2468)
	{
		while(1)
		{
			LEDS_ALL_OFF;
			LED_0_ON;LED_2_ON;LED_4_ON;LED_6_ON;
			vTaskDelay(delay / portTICK_PERIOD_MS);
			LEDS_ALL_OFF;
			LED_1_ON;LED_3_ON;LED_5_ON;LED_7_ON;
			vTaskDelay(delay / portTICK_PERIOD_MS);
		}
	}
	if(pattern == ERROR_BLINK_PATTERN_1256_3478)
	{
		while(1)
		{
			LEDS_ALL_OFF;
			LED_0_ON;LED_1_ON;LED_4_ON;LED_5_ON;
			vTaskDelay(delay / portTICK_PERIOD_MS);
			LEDS_ALL_OFF;
			LED_2_ON;LED_3_ON;LED_6_ON;LED_7_ON;
			vTaskDelay(delay / portTICK_PERIOD_MS);
		}
	}
}

void set_led(int led, int state)
{
	if(led==0) { if(state) { LED_8_ON; } else { LED_8_OFF; } }
	if(led==1) { if(state) { LED_0_ON; } else { LED_0_OFF; } }
	if(led==2) { if(state) { LED_9_ON; } else { LED_9_OFF; } }
	if(led==3) { if(state) { LED_1_ON; } else { LED_1_OFF; } }
	if(led==4) { if(state) { LED_10_ON; } else { LED_10_OFF; } }
	if(led==5) { if(state) { LED_2_ON; } else { LED_2_OFF; } }
	if(led==6) { if(state) { LED_11_ON; } else { LED_11_OFF; } }
	if(led==7) { if(state) { LED_3_ON; } else { LED_3_OFF; } }

	if(led==8) { if(state) { LED_12_ON; } else { LED_12_OFF; } }
	if(led==10) { if(state) { LED_13_ON; } else { LED_13_OFF; } }
	if(led==12) { if(state) { LED_14_ON; } else { LED_14_OFF; } }
	if(led==14) { if(state) { LED_15_ON; } else { LED_15_OFF; } }
	if(led==9) { if(state) { LED_4_ON; } else { LED_4_OFF; } }
	if(led==11) { if(state) { LED_5_ON; } else { LED_5_OFF; } }
	if(led==13) { if(state) { LED_6_ON; } else { LED_6_OFF; } }
	if(led==15) { if(state) { LED_7_ON; } else { LED_7_OFF; } }
}

/*
void divide_sequencer_steps(int steps, int8_t *r1, int8_t *r2)
{
	int row1, row2;
	for(row1=1;row1<=8;row1++)
	{
		for(row2=1;row2<=8;row2++)
		{
			if(row1*row2==steps)
			{
				r1[0] = row1;
				r2[0] = row2;
				return;
			}
		}
	}
	printf("divide_sequencer_steps(%d, ...): could not find a solution\n", steps);

	//set to 16 steps as default
	r1[0] = 4;
	r2[0] = 4;
}
*/

void indicate_sequencer_steps(int row1, int row2)
{
	LEDS_display_direct(~(0xff00>>(8-row1)),(0xff>>(8-row2)));
}
