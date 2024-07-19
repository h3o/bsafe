/*
 * init.c
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 23 Jan 2018
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

#include "init.h"
#include "gpio.h"
#include "signals.h"
#include "keys.h"
//#include "esp32-hal-dac.h"
#include "settings.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

//#define DEBUG_OUTPUT
//#define LIGHT_SENSORS_TEST

int8_t ENCODER_STEPS_PER_EVENT = ENCODER_STEPS_PER_EVENT_DEFAULT;

size_t i2s_bytes_rw;

int sensors_active = SENSORS_ACTIVE_RIGHT, sensors_settings_shift = 4, sensors_settings_div = 1;

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void init_deinit_TWDT()
{
	printf("Initialize TWDT\n");
	//Initialize or reinitialize TWDT
	//CHECK_ERROR_CODE(esp_task_wdt_init(TWDT_TIMEOUT_S, false), ESP_OK);
	CHECK_ERROR_CODE(esp_task_wdt_init(5, false), ESP_OK);

	//Subscribe Idle Tasks to TWDT if they were not subscribed at startup
	#ifndef CONFIG_TASK_WDT_CHECK_IDLE_TASK_CPU0
	esp_task_wdt_add(xTaskGetIdleTaskHandleForCPU(0));
	#endif
	#ifndef CONFIG_TASK_WDT_CHECK_IDLE_TASK_CPU1
	esp_task_wdt_add(xTaskGetIdleTaskHandleForCPU(1));
	#endif

    //unsubscribe idle tasks
    CHECK_ERROR_CODE(esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0)), ESP_OK);     //Unsubscribe Idle Task from TWDT
    CHECK_ERROR_CODE(esp_task_wdt_status(xTaskGetIdleTaskHandleForCPU(0)), ESP_ERR_NOT_FOUND);      //Confirm Idle task has unsubscribed

    CHECK_ERROR_CODE(esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1)), ESP_OK);     //Unsubscribe Idle Task from TWDT
    CHECK_ERROR_CODE(esp_task_wdt_status(xTaskGetIdleTaskHandleForCPU(1)), ESP_ERR_NOT_FOUND);      //Confirm Idle task has unsubscribed

    //Deinit TWDT after all tasks have unsubscribed
    CHECK_ERROR_CODE(esp_task_wdt_deinit(), ESP_OK);
    CHECK_ERROR_CODE(esp_task_wdt_status(NULL), ESP_ERR_INVALID_STATE);     //Confirm TWDT has been deinitialized

    printf("TWDT Deinitialized\n");
}

float micros()
{
    static struct timeval time_of_day;
    gettimeofday(&time_of_day, NULL);
    return time_of_day.tv_sec + time_of_day.tv_usec / 1000000.0;
}

uint32_t micros_i()
{
    static struct timeval time_of_day;
    gettimeofday(&time_of_day, NULL);
    return 1000000.0 * time_of_day.tv_sec + time_of_day.tv_usec;
}

uint32_t millis()
{
    static struct timeval time_of_day;
    gettimeofday(&time_of_day, NULL);
    return 1000.0 * time_of_day.tv_sec + time_of_day.tv_usec / 1000;
}

#ifdef BOARD_BSAF_V1
const char* FW_VERSION = "[Bv1/1.0.001]";
#endif

void generate_random_seed()
{
	//randomize the pseudo RNG seed
	bootloader_random_enable();
	set_pseudo_random_seed((double)esp_random() / (double)UINT32_MAX);
	bootloader_random_disable();
}

void LEDs_init(int core)
{
	int result;

	//anodes

	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_NUM_9], FUNC_SD_DATA2_GPIO9); //normally SD2
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_NUM_10], FUNC_SD_DATA3_GPIO10); //normally SD3

	result = gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
	printf("GPIO5 direction set result = %d\n",result);

	result = gpio_set_direction(GPIO_NUM_9, GPIO_MODE_OUTPUT); //SD2
	printf("GPIO9 direction set result = %d\n",result);

	result = gpio_set_direction(GPIO_NUM_10, GPIO_MODE_OUTPUT); //SD3
	printf("GPIO10 direction set result = %d\n",result);

	result = gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
	printf("GPIO18 direction set result = %d\n",result);

	result = gpio_set_pull_mode(GPIO_NUM_9, GPIO_FLOATING);
	printf("GPI9 pull mode set result = %d\n",result);

	result = gpio_set_pull_mode(GPIO_NUM_10, GPIO_FLOATING);
	printf("GPI10 pull mode set result = %d\n",result);

	gpio_set_level(GPIO_NUM_5, 0); //off
	gpio_set_level(GPIO_NUM_9, 0); //off
	gpio_set_level(GPIO_NUM_10, 0); //off
	gpio_set_level(GPIO_NUM_18, 0); //off

	//cathodes

	result = gpio_set_direction(GPIO_NUM_19, GPIO_MODE_OUTPUT);
	printf("GPIO19 direction set result = %d\n",result);

	result = gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
	printf("GPIO21 direction set result = %d\n",result);

	result = gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT);
	printf("GPIO22 direction set result = %d\n",result);

	result = gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT);
	printf("GPIO23 direction set result = %d\n",result);

	gpio_set_level(GPIO_NUM_19, 1); //off
	gpio_set_level(GPIO_NUM_21, 1); //off
	gpio_set_level(GPIO_NUM_22, 1); //off
	gpio_set_level(GPIO_NUM_23, 1); //off

	#ifndef LEDS_TEST
	printf("LEDs_init(): starting task [LEDs_driver_task]\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&LEDs_driver_task, "LEDs_driver", 4096, NULL, 10, NULL, core);
	#endif
}

#ifdef LEDS_TEST

void LEDs_test_task(void *pvParameters)
{
	printf("LEDs_test_task() starting\n");

	while(1)
	{
		/*
		//orange
		LED_0_ON; usDelay(LED_TEST_DELAY);LED_0_OFF;
		LED_1_ON; usDelay(LED_TEST_DELAY);LED_1_OFF;
		LED_2_ON; usDelay(LED_TEST_DELAY);LED_2_OFF;
		LED_3_ON; usDelay(LED_TEST_DELAY);LED_3_OFF;

		//blue
		LED_4_ON; usDelay(LED_TEST_DELAY/3);LED_4_OFF;
		LED_5_ON; usDelay(LED_TEST_DELAY/3);LED_5_OFF;
		LED_6_ON; usDelay(LED_TEST_DELAY/3);LED_6_OFF;
		LED_7_ON; usDelay(LED_TEST_DELAY/3);LED_7_OFF;

		//green
		LED_8_ON; usDelay(LED_TEST_DELAY/6);LED_8_OFF;
		LED_9_ON; usDelay(LED_TEST_DELAY/6);LED_9_OFF;
		LED_10_ON; usDelay(LED_TEST_DELAY/6);LED_10_OFF;
		LED_11_ON; usDelay(LED_TEST_DELAY/6);LED_11_OFF;

		//yellow
		LED_12_ON; usDelay(LED_TEST_DELAY*2);LED_12_OFF;
		LED_13_ON; usDelay(LED_TEST_DELAY*2);LED_13_OFF;
		LED_14_ON; usDelay(LED_TEST_DELAY*2);LED_14_OFF;
		LED_15_ON; usDelay(LED_TEST_DELAY*2);LED_15_OFF;
		*/

		//for better balanced LEDs:
		LED_0_ON; usDelay(LED_TEST_DELAY);LED_0_OFF;
		LED_4_ON; usDelay(LED_TEST_DELAY);LED_4_OFF;
		LED_8_ON; usDelay(LED_TEST_DELAY);LED_8_OFF;
		LED_12_ON; usDelay(LED_TEST_DELAY);LED_12_OFF;
	}
}

void LEDs_test()
{
	printf("LEDs_test(): starting task [LEDs_test_task]\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&LEDs_test_task, "LEDs_test_task", 4096, NULL, 10, NULL, 1);
}

#else

uint8_t led_disp[7] = {4,0,1,4,1,0,0}; //LEDS_BLUE1,LEDS_ORANGE2,LEDS_ORANGE1,LEDS_BLUE2,LEDS_ORANGE,LEDS_DIRECT_BLUE,LEDS_DIRECT_ORANGE
int led_indication_refresh = -1;
int menu_function = MENU_PLAY;//MENU_FRM;
int enc_function = ENC_FUNCTION_NONE;
int enc_tempo_mult = 0;
int led_display_direct = 0, led_display_direct_blink_blue = 0, led_display_direct_blink_orange = 0, led_direct_blink_cycle = 0;
#define LED_DIRECT_BLINK_SPEED	0x08

void LEDs_driver_task(void *pvParameters)
{
	printf("LEDs_driver_task() starting\n");

	int led_cycle = 0;

	while(1)
	{
		if(led_display_direct)
		{
			//LEDS_BLUE1
			if(led_display_direct_blink_blue!=1||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_BLUE]&0x01)LED_0_ON;Delay(LED_DIRECT_DELAY);LED_0_OFF;
			if(led_display_direct_blink_blue!=2||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_BLUE]&0x02)LED_1_ON;Delay(LED_DIRECT_DELAY);LED_1_OFF;
			if(led_display_direct_blink_blue!=3||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_BLUE]&0x04)LED_2_ON;Delay(LED_DIRECT_DELAY);LED_2_OFF;
			if(led_display_direct_blink_blue!=4||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_BLUE]&0x08)LED_3_ON;Delay(LED_DIRECT_DELAY);LED_3_OFF;
			//LEDS_ORANGE2
			if(led_display_direct_blink_orange!=5||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_ORANGE]&0x10)LED_4_ON;Delay(LED_DIRECT_DELAY);LED_4_OFF;
			if(led_display_direct_blink_orange!=6||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_ORANGE]&0x20)LED_5_ON;Delay(LED_DIRECT_DELAY);LED_5_OFF;
			if(led_display_direct_blink_orange!=7||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_ORANGE]&0x40)LED_6_ON;Delay(LED_DIRECT_DELAY);LED_6_OFF;
			if(led_display_direct_blink_orange!=8||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_ORANGE]&0x80)LED_7_ON;Delay(LED_DIRECT_DELAY);LED_7_OFF;
			//LEDS_ORANGE1
			if(led_display_direct_blink_orange!=1||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_ORANGE]&0x01)LED_8_ON;Delay(LED_DIRECT_DELAY);LED_8_OFF;
			if(led_display_direct_blink_orange!=2||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_ORANGE]&0x02)LED_9_ON;Delay(LED_DIRECT_DELAY);LED_9_OFF;
			if(led_display_direct_blink_orange!=3||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_ORANGE]&0x04)LED_10_ON;Delay(LED_DIRECT_DELAY);LED_10_OFF;
			if(led_display_direct_blink_orange!=4||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_ORANGE]&0x08)LED_11_ON;Delay(LED_DIRECT_DELAY);LED_11_OFF;
			//LEDS_BLUE2
			if(led_display_direct_blink_blue!=5||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_BLUE]&0x10)LED_12_ON;Delay(LED_DIRECT_DELAY);LED_12_OFF;
			if(led_display_direct_blink_blue!=6||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_BLUE]&0x20)LED_13_ON;Delay(LED_DIRECT_DELAY);LED_13_OFF;
			if(led_display_direct_blink_blue!=7||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_BLUE]&0x40)LED_14_ON;Delay(LED_DIRECT_DELAY);LED_14_OFF;
			if(led_display_direct_blink_blue!=8||(led_direct_blink_cycle&LED_DIRECT_BLINK_SPEED))
			if(led_disp[LEDS_DIRECT_BLUE]&0x80)LED_15_ON;Delay(LED_DIRECT_DELAY);LED_15_OFF;

			led_direct_blink_cycle++;
		}
		else
		{
			if(led_cycle%4==0)
			{
				//LEDS_BLUE1
				if((led_disp[0]==1+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[0]==1)) { LED_0_ON; }
				if((led_disp[0]==2+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[0]==2)) { LED_1_ON; }
				if((led_disp[0]==3+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[0]==3)) { LED_2_ON; }
				if((led_disp[0]==4+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[0]==4)) { LED_3_ON; }
			}

			if(led_cycle%4==1)
			{
				//LEDS_ORANGE2
				if((led_disp[1]==1+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[1]==1)) { LED_4_ON; }
				if((led_disp[1]==2+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[1]==2)) { LED_5_ON; }
				if((led_disp[1]==3+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[1]==3)) { LED_6_ON; }
				if((led_disp[1]==4+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[1]==4)) { LED_7_ON; }
			}

			if(led_cycle%4==2)
			{
				//LEDS_ORANGE1
				if((led_disp[2]==1+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[2]==1)) { LED_8_ON; }
				if((led_disp[2]==2+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[2]==2)) { LED_9_ON; }
				if((led_disp[2]==3+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[2]==3)) { LED_10_ON; }
				if((led_disp[2]==4+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[2]==4)) { LED_11_ON; }
			}

			if(led_cycle%4==3)
			{
				//LEDS_BLUE2
				if((led_disp[3]==1+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[3]==1)) { LED_12_ON; }
				if((led_disp[3]==2+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[3]==2)) { LED_13_ON; }
				if((led_disp[3]==3+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[3]==3)) { LED_14_ON; }
				if((led_disp[3]==4+LEDS_BLINK && led_cycle<LED_BLINK_DELAY) || (led_disp[3]==4)) { LED_15_ON; }
			}

			Delay(2);

			LEDS_ALL_OFF;

			led_cycle++;
			if(led_cycle==LED_BLINK_DELAY*2)
			{
				led_cycle=0;
			}
		}
	}
}

#endif

void LEDS_display_direct(uint8_t bitmap_orange, uint8_t bitmap_blue)
{
	led_disp[LEDS_DIRECT_BLUE] = bitmap_blue;
	led_disp[LEDS_DIRECT_ORANGE] = bitmap_orange;
	led_display_direct = 1;
}

void LEDS_display_direct_blink(int blink_orange, int blink_blue)
{
	led_display_direct_blink_orange = blink_orange;
	led_display_direct_blink_blue = blink_blue;
	led_direct_blink_cycle = 0;
}

void LEDS_display_direct_end()
{
	led_display_direct = 0;
	led_disp[LEDS_DIRECT_BLUE] = 0;
	led_disp[LEDS_DIRECT_ORANGE] = 0;
	led_display_direct_blink_blue = 0;
	led_display_direct_blink_orange = 0;
}

int8_t encoder_events[4] = {-1,-1,-1,-1}; //uninitialized
int8_t encoder_counters[2] = {0,0};
int8_t encoder_results[2] = {0,0};
//int8_t encoder_event[2] = {0,0};

#define ENCODERS_CYCLE_DELAY	1 //ms
#define ENCODERS_CYCLE_TIMING	10 //20 //multiplier for ENCODERS_CYCLE_DELAY

void LEDS_display_load_save_animation(int position, int speed, int delays)
{
	LEDS_display_direct(0,0x01<<position); //orange off, blue shows selected position
	Delay(delays);
	for(int i=8;i>=0;i--)
	{
		LEDS_display_direct(~((uint16_t)0xff00>>i),0x01<<position); //orange animation
		Delay(speed);
	}
	Delay(delays);

	LEDS_display_direct_end();
	LEDS_BLUE_OFF;
}

void encoders_task(void *pvParameters)
{
	printf("encoders_task() starting\n");

	int val[4], enc_pair;
	int result, cycle_ms = 0;

	result = gpio_set_direction(GPIO_NUM_36, GPIO_MODE_INPUT);
	printf("GPIO36 direction set result = %d\n",result);
	result = gpio_set_direction(GPIO_NUM_37, GPIO_MODE_INPUT);
	printf("GPIO37 direction set result = %d\n",result);
	result = gpio_set_direction(GPIO_NUM_38, GPIO_MODE_INPUT);
	printf("GPIO38 direction set result = %d\n",result);
	result = gpio_set_direction(GPIO_NUM_39, GPIO_MODE_INPUT);
	printf("GPIO39 direction set result = %d\n",result);

	//these pins are input only and do not have pullup/pulldown circuitry
	//result = gpio_set_pull_mode(GPIO_NUM_38, GPIO_PULLUP_PULLDOWN);
	//result = gpio_set_pull_mode(GPIO_NUM_38, GPIO_PULLUP_ONLY);
	//result = gpio_set_pull_mode(GPIO_NUM_38, GPIO_PULLDOWN_ONLY);
	//result = gpio_set_pull_mode(GPIO_NUM_38, GPIO_FLOATING);
	//printf("GPI38 pull mode set result = %d\n",result);

	#ifdef ENCODERS_ANALOG
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); //GPI36 is connected to ADC1 channel #0
	adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11); //GPI37 is connected to ADC1 channel #1
	adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11); //GPI38 is connected to ADC1 channel #2
	adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11); //GPI39 is connected to ADC1 channel #3
	#endif

	while(1)
	{
		cycle_ms++;

		#ifdef ENCODERS_ANALOG
		val[0] = adc1_get_raw(ADC1_CHANNEL_0);
		val[1] = adc1_get_raw(ADC1_CHANNEL_1);
		val[2] = adc1_get_raw(ADC1_CHANNEL_2);
		val[3] = adc1_get_raw(ADC1_CHANNEL_3);
		#else
		//encoder #1
		val[0] = gpio_get_level(GPIO_NUM_38); //reversed
		val[2] = gpio_get_level(GPIO_NUM_36);
		//encoder #2
		val[1] = gpio_get_level(GPIO_NUM_37);
		val[3] = gpio_get_level(GPIO_NUM_39);
		#endif

		//printf("GPI36,37,38,39 values: %d-%d-%d-%d\n", val[0], val[1], val[2], val[3]);

		if(encoder_events[0]==-1) //startup
		{
			encoder_events[0] = val[0];
			encoder_events[1] = val[1];
			encoder_events[2] = val[2];
			encoder_events[3] = val[3];
		}
		else
		{
			for(enc_pair=0;enc_pair<2;enc_pair++)
			{
				if(encoder_events[enc_pair+0] != val[enc_pair+0] || encoder_events[enc_pair+2] != val[enc_pair+2]) //left changed
				{
					if(encoder_events[enc_pair+0]==1 && val[enc_pair+0]==0 && encoder_events[enc_pair+2]==1 && val[enc_pair+2]==1)
					{
						encoder_counters[enc_pair] += ENCODER_EVENT_DEC;
					}
					if(encoder_events[enc_pair+2]==1 && val[enc_pair+2]==0 && encoder_events[enc_pair+0]==0 && val[enc_pair+0]==0)
					{
						encoder_counters[enc_pair] += ENCODER_EVENT_DEC;
					}
					if(encoder_events[enc_pair+0]==0 && val[enc_pair+0]==1 && encoder_events[enc_pair+2]==0 && val[enc_pair+2]==0)
					{
						encoder_counters[enc_pair] += ENCODER_EVENT_DEC;
					}
					if(encoder_events[enc_pair+2]==0 && val[enc_pair+2]==1 && encoder_events[enc_pair+0]==1 && val[enc_pair+0]==1)
					{
						encoder_counters[enc_pair] += ENCODER_EVENT_DEC;
					}
					if(encoder_events[enc_pair+0]==1 && val[enc_pair+0]==0 && encoder_events[enc_pair+2]==0 && val[enc_pair+2]==0)
					{
						encoder_counters[enc_pair] += ENCODER_EVENT_INC;
					}
					if(encoder_events[enc_pair+2]==1 && val[enc_pair+2]==0 && encoder_events[enc_pair+0]==1 && val[enc_pair+0]==1)
					{
						encoder_counters[enc_pair] += ENCODER_EVENT_INC;
					}
					if(encoder_events[enc_pair+0]==0 && val[enc_pair+0]==1 && encoder_events[enc_pair+2]==1 && val[enc_pair+2]==1)
					{
						encoder_counters[enc_pair] += ENCODER_EVENT_INC;
					}
					if(encoder_events[enc_pair+2]==0 && val[enc_pair+2]==1 && encoder_events[enc_pair+0]==0 && val[enc_pair+0]==0)
					{
						encoder_counters[enc_pair] += ENCODER_EVENT_INC;
					}
				}
			}

			encoder_events[0] = val[0];
			encoder_events[1] = val[1];
			encoder_events[2] = val[2];
			encoder_events[3] = val[3];
		}

		if(cycle_ms%ENCODERS_CYCLE_TIMING==0)
		{
			//test indicate the result

			if(encoder_counters[ENCODER_LEFT]>=ENCODER_STEPS_PER_EVENT || encoder_counters[ENCODER_LEFT]<=-ENCODER_STEPS_PER_EVENT)
			{
				#ifdef ENCODERS_TEST
				printf("--------------------> LEFT ENCODER result = %d", encoder_counters[ENCODER_LEFT]);
				#endif
				if(encoder_counters[ENCODER_LEFT]>0)
				{
					#ifdef ENCODERS_TEST
					printf(" (INCREASE)\n");
					#endif
					encoder_results[ENCODER_LEFT] = 1;
					encoder_changed++;
				}
				if(encoder_counters[ENCODER_LEFT]<0)
				{
					#ifdef ENCODERS_TEST
					printf(" (DECREASE)\n");
					#endif
					encoder_results[ENCODER_LEFT] = -1;
					encoder_changed++;
				}
				encoder_counters[ENCODER_LEFT] = 0;
			}
			if(encoder_counters[ENCODER_RIGHT]>=ENCODER_STEPS_PER_EVENT || encoder_counters[ENCODER_RIGHT]<=-ENCODER_STEPS_PER_EVENT)
			{
				#ifdef ENCODERS_TEST
				printf("--------------------> RIGHT ENCODER result = %d", encoder_counters[ENCODER_RIGHT]);
				#endif
				if(encoder_counters[ENCODER_RIGHT]>0)
				{
					#ifdef ENCODERS_TEST
					printf(" (INCREASE)\n");
					#endif
					encoder_results[ENCODER_RIGHT] = 1;
					encoder_changed++;
				}
				if(encoder_counters[ENCODER_RIGHT]<0)
				{
					#ifdef ENCODERS_TEST
					printf(" (DECREASE)\n");
					#endif
					encoder_results[ENCODER_RIGHT] = -1;
					encoder_changed++;
				}
				encoder_counters[ENCODER_RIGHT] = 0;
			}
		}

		Delay(ENCODERS_CYCLE_DELAY);
	}
}

void encoders_init(int core)
{
	printf("encoders_init()\n");

	ENCODER_STEPS_PER_EVENT = get_encoder_steps_per_event();
	printf("encoders_init(): ENCODER_STEPS_PER_EVENT loaded value = %d\n", ENCODER_STEPS_PER_EVENT);
	if(ENCODER_STEPS_PER_EVENT<1 || ENCODER_STEPS_PER_EVENT>8)
	{
		ENCODER_STEPS_PER_EVENT = ENCODER_STEPS_PER_EVENT_DEFAULT;
		printf("encoders_init(): ENCODER_STEPS_PER_EVENT value out of range, setting to default => %d\n", ENCODER_STEPS_PER_EVENT);
	}

	printf("encoders_init(): starting task [encoders_task]\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&encoders_task, "encoders_task", 4096, NULL, 10, NULL, core);

	/*
	//test encoders
	int val[4];
	while(1)
	{
		val[0] = adc1_get_raw(ADC1_CHANNEL_0);
		val[1] = adc1_get_raw(ADC1_CHANNEL_1);
		val[2] = adc1_get_raw(ADC1_CHANNEL_2);
		val[3] = adc1_get_raw(ADC1_CHANNEL_3);
		printf("GPI36,37,38,39 analog values: %d-%d-%d-%d\n", val[0], val[1], val[2], val[3]);
		Delay(100);
	}
	*/
}

#ifdef SENSORS_FLOAT
float light_sensor_results[2] = {0,0};
#else
int16_t light_sensor_results[2] = {0,0};
#endif

float sensor_range = SENSOR_RANGE_DEFAULT;
int32_t sensor_base = SENSOR_BASE_DEFAULT;
uint8_t sensors_wt_layers = 0; //in wave table layers, by default both sensors off

void light_sensors_task(void *pvParameters)
{
	printf("light_sensors_task() starting\n");

	int result, cycle_ms = 0;

	result = gpio_set_direction(GPIO_NUM_34, GPIO_MODE_INPUT);
	printf("GPIO34 direction set result = %d\n",result);
	result = gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT);
	printf("GPIO35 direction set result = %d\n",result);

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); //GPI34 is connected to ADC1 channel #6
	adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11); //GPI35 is connected to ADC1 channel #7

	while(1)
	{
		cycle_ms++;

		#ifdef LIGHT_SENSOR_LPF_ALPHA
		light_sensor_results[0] += LIGHT_SENSOR_LPF_ALPHA * ((float)(adc1_get_raw(ADC1_CHANNEL_6) - light_sensor_results[0]));
		light_sensor_results[1] += LIGHT_SENSOR_LPF_ALPHA * ((float)(adc1_get_raw(ADC1_CHANNEL_7) - light_sensor_results[1]));
		#else
		light_sensor_results[0] = adc1_get_raw(ADC1_CHANNEL_6);
		light_sensor_results[1] = adc1_get_raw(ADC1_CHANNEL_7);
		#endif

		//printf("GPI34,35 values: %d-%d\n", val[0], val[1]);

		if(cycle_ms%10==0)
		{
			#ifdef LIGHT_SENSORS_TEST
			printf("GPI34,35 values:	%f	%f\n", light_sensor_results[0], light_sensor_results[1]);
			#endif
		}

		Delay(5);
	}
}

void light_sensors_init(int core)
{
	//printf("light_sensors_init()\n");

	printf("light_sensors_init(): starting task [light_sensors_task]\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&light_sensors_task, "light_sensors_task", 4096, NULL, 10, NULL, core);
}

/*
void IRAM_ATTR dacWrite(uint8_t pin, uint8_t value)
{
    if(pin < 25 || pin > 26){
        return;//not dac pin
    }
    //pinMode(pin, ANALOG);
    uint8_t channel = pin - 25;


    //Disable Tone
    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);

    if (channel) {
        //Disable Channel Tone
        CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
        //Set the Dac value
        SET_PERI_REG_BITS(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, value, RTC_IO_PDAC2_DAC_S);   //dac_output
        //Channel output enable
        SET_PERI_REG_MASK(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_XPD_DAC | RTC_IO_PDAC2_DAC_XPD_FORCE);
    } else {
        //Disable Channel Tone
        CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
        //Set the Dac value
        SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, value, RTC_IO_PDAC1_DAC_S);   //dac_output
        //Channel output enable
        SET_PERI_REG_MASK(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_XPD_DAC | RTC_IO_PDAC1_DAC_XPD_FORCE);
    }
}
*/

//extern esp_err_t adc_set_i2s_data_len(adc_unit_t adc_unit, int patt_len);
//extern esp_err_t adc_set_i2s_data_pattern(adc_unit_t adc_unit, int seq_num, adc_channel_t channel, adc_bits_width_t bits, adc_atten_t atten);

//#define ADC_I2S_INPUT

void DAC_init()
{
	printf("DAC_init()\n");

	i2s_config_t cfg={

		#ifdef ADC_I2S_INPUT
		.mode=I2S_MODE_DAC_BUILT_IN|I2S_MODE_TX|I2S_MODE_MASTER|I2S_MODE_ADC_BUILT_IN|I2S_MODE_RX,
		#else
		.mode=I2S_MODE_DAC_BUILT_IN|I2S_MODE_TX|I2S_MODE_MASTER,
		#endif

		.sample_rate=I2S_AUDIOFREQ,
		//.bits_per_sample=I2S_BITS_PER_SAMPLE_8BIT,
		.bits_per_sample=I2S_BITS_PER_SAMPLE_16BIT,
		.channel_format=I2S_CHANNEL_FMT_RIGHT_LEFT,
		//.communication_format= I2S_COMM_FORMAT_I2S_LSB,//MSB, //seems to have no effect with these parameters
		.communication_format=I2S_COMM_FORMAT_PCM,//|I2S_COMM_FORMAT_PCM_SHORT,
		.intr_alloc_flags=0,
		.dma_buf_count=16,
		.dma_buf_len=16,
		.use_apll=false//true
	};

	i2s_driver_install(I2S_NUM, &cfg, 0, NULL);//4, &soundQueue);
	i2s_set_pin(I2S_NUM, NULL);
	i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);//I2S_DAC_CHANNEL_LEFT_EN);
	//i2s_set_sample_rates(I2S_NUM, cfg.sample_rate);

	#ifdef ADC_I2S_INPUT
	i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_6);
	//adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0); //GPIO34 is connected to ADC1 channel #6
	i2s_adc_enable(I2S_NUM_0);

	// Expand default pattern with the clk channel at index 1
	// Data is assigned to pattern index 0 by i2s_set_adc_mode
	//adc_set_i2s_data_len(I2S_NUM_0, 2);
	//adc_set_i2s_data_pattern(I2S_NUM_0, 0, ADC1_CHANNEL_6, ADC_WIDTH_BIT_12, ADC_ATTEN_DB_0);
	//adc_set_i2s_data_pattern(I2S_NUM_0, 1, ADC1_CHANNEL_7, ADC_WIDTH_BIT_12, ADC_ATTEN_DB_0);}
	#endif
}

void DAC_test()
{
	printf("DAC_test()\n");

	#define BUFFSIZE 100
	uint32_t tmpb[BUFFSIZE];
	uint8_t s1, s2;

	for (int j=0; j<BUFFSIZE; j++) {

		s1=j;//(put your sample calculation here);
		s2=BUFFSIZE-j;//(put your sample calculation here);

		tmpb[j]=((s1)<<2)+((s2)<<18);
		//tmpb[j]=((s1)<<8);//+((s2)<<24);
		//tmpb[j]=((s2)<<24);
	}

	size_t written;

	while(1)
	{
		i2s_write(I2S_NUM, (char*)tmpb, BUFFSIZE*4, &written, portMAX_DELAY);
	}
}

void send_silence(int samples)
{
	size_t written;
	uint32_t zero = 0;
	for(int i=0;i<samples/4;i++)
	{
		i2s_write(I2S_NUM, (char*)&zero, 4, &written, portMAX_DELAY);
	}
}


void MCU_restart()
{
	//codec_set_mute(1); //mute the codec
	//Delay(20);
	//codec_reset();
	//Delay(10);
	esp_restart();
	while(1);
}

/*
void brownout_init()
{
	#define BROWNOUT_DET_LVL 0

	REG_WRITE(RTC_CNTL_BROWN_OUT_REG,
            RTC_CNTL_BROWN_OUT_ENA // Enable BOD
            | RTC_CNTL_BROWN_OUT_PD_RF_ENA // Automatically power down RF
            //Reset timeout must be set to >1 even if BOR feature is not used
            | (2 << RTC_CNTL_BROWN_OUT_RST_WAIT_S)
            | (BROWNOUT_DET_LVL << RTC_CNTL_DBROWN_OUT_THRES_S));

    rtc_isr_register(low_voltage_poweroff, NULL, RTC_CNTL_BROWN_OUT_INT_ENA_M);
    printf("Initialized BOD\n");

    REG_SET_BIT(RTC_CNTL_INT_ENA_REG, RTC_CNTL_BROWN_OUT_INT_ENA_M);
}
*/

//https://github.com/espressif/esp-idf/blob/master/examples/peripherals/touch_pad_read/main/esp32/tp_read_main.c

#define TOUCH_PAD_NO_CHANGE   (-1)
#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_FILTER_MODE_EN  (0) //(1)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

int touchpad_state[10] = {0,0,0,0,0,0,0,0,0,0};
int touchpad_event[10] = {0,0,0,0,0,0,0,0,0,0};
int touchpad_released[10] = {0,0,0,0,0,0,0,0,0,0};
#define TOUCHPAD_RELEASE_TIMEOUT	0 //2

int bottom_key_pressed = 0, set_pressed = 0, shift_pressed = 0, set_held = 0, shift_held = 0, shift_clicked = 0, set_clicked = 0;

//cca 25 increments per second
#define SET_HELD_TIMEOUT			25*2 //2 sec
#define SHIFT_HELD_TIMEOUT			25*2 //2 sec
int touchpad_event_SET_HELD = 0, touchpad_event_SHIFT_HELD = 0, touchpad_event_BOTH_HELD = 0;
int encoder_changed = 0;
int keys_or_encoders_recently = 0;
int last_ui_event = UI_EVENT_NONE;

void touch_pad_scan(void *pvParameters)
{
	esp_err_t res;
	uint16_t touch_value;

	// Initialize touch pad peripheral.
	// The default fsm mode is software trigger mode.
	res = touch_pad_init();
	printf("touch_pad_test(): touch_pad_init returned code %d\n", res);

	// Set reference voltage for charging/discharging
	// In this case, the high reference voltage will be 2.7V - 1V = 1.7V
	// The low reference voltage will be 0.5
	// The larger the range, the larger the pulse count value.
	res = touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
	//res = touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_KEEP);
	//res = touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_0V);
	printf("touch_pad_test(): touch_pad_set_voltage returned code %d\n", res);

	res = touch_pad_config(TOUCH_PAD_NUM0, TOUCH_THRESH_NO_USE);
	res = touch_pad_config(TOUCH_PAD_NUM1, TOUCH_THRESH_NO_USE);
	res = touch_pad_config(TOUCH_PAD_NUM2, TOUCH_THRESH_NO_USE);
	res = touch_pad_config(TOUCH_PAD_NUM3, TOUCH_THRESH_NO_USE);
	res = touch_pad_config(TOUCH_PAD_NUM4, TOUCH_THRESH_NO_USE);
	res = touch_pad_config(TOUCH_PAD_NUM5, TOUCH_THRESH_NO_USE);
	res = touch_pad_config(TOUCH_PAD_NUM6, TOUCH_THRESH_NO_USE);
	res = touch_pad_config(TOUCH_PAD_NUM7, TOUCH_THRESH_NO_USE);
	res = touch_pad_config(TOUCH_PAD_NUM8, TOUCH_THRESH_NO_USE);
	res = touch_pad_config(TOUCH_PAD_NUM9, TOUCH_THRESH_NO_USE);

	res = touch_pad_set_cnt_mode(TOUCH_PAD_NUM0, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);
	res = touch_pad_set_cnt_mode(TOUCH_PAD_NUM1, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);
	res = touch_pad_set_cnt_mode(TOUCH_PAD_NUM2, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);
	res = touch_pad_set_cnt_mode(TOUCH_PAD_NUM3, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);
	res = touch_pad_set_cnt_mode(TOUCH_PAD_NUM4, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);
	res = touch_pad_set_cnt_mode(TOUCH_PAD_NUM5, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);
	res = touch_pad_set_cnt_mode(TOUCH_PAD_NUM6, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);
	res = touch_pad_set_cnt_mode(TOUCH_PAD_NUM7, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);
	res = touch_pad_set_cnt_mode(TOUCH_PAD_NUM8, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);
	res = touch_pad_set_cnt_mode(TOUCH_PAD_NUM9, TOUCH_PAD_SLOPE_7, TOUCH_PAD_TIE_OPT_LOW);

	while(1)
	{
		/*
		touch_pad_read(TOUCH_PAD_NUM0, &touch_value);
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { touchpad_state[0]=1; } else { touchpad_state[0]=-1; }

		touch_pad_read(TOUCH_PAD_NUM1, &touch_value);
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD_SHIFT) { touchpad_state[1]=1; } else { touchpad_state[1]=-1; }

		touch_pad_read(TOUCH_PAD_NUM2, &touch_value);
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { touchpad_state[2]=1; } else { touchpad_state[2]=-1; }

		touch_pad_read(TOUCH_PAD_NUM3, &touch_value);
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { touchpad_state[3]=1; } else { touchpad_state[3]=-1; }

		touch_pad_read(TOUCH_PAD_NUM4, &touch_value);
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { touchpad_state[4]=1; } else { touchpad_state[4]=-1; }

		touch_pad_read(TOUCH_PAD_NUM5, &touch_value);
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { touchpad_state[5]=1; } else { touchpad_state[5]=-1; }

		touch_pad_read(TOUCH_PAD_NUM6, &touch_value);
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { touchpad_state[6]=1; } else { touchpad_state[6]=-1; }

		touch_pad_read(TOUCH_PAD_NUM7, &touch_value);
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { touchpad_state[7]=1; } else { touchpad_state[7]=-1; }

		touch_pad_read(TOUCH_PAD_NUM8, &touch_value);
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { touchpad_state[8]=1; } else { touchpad_state[8]=-1; }

		touch_pad_read(TOUCH_PAD_NUM9, &touch_value);
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { touchpad_state[9]=1; } else { touchpad_state[9]=-1; }
		*/

		//for(int pad=0; pad<TOUCH_PAD_MAX; pad++)
		for(int pad=TOUCH_PAD_MAX-1; pad>=0; pad--)
		{
			touch_pad_read(pad, &touch_value);
			//printf("%d\t-\t",touch_value);
			if(touch_value<(pad==TOUCH_PAD_RIGHT?TOUCHPAD_PRESS_THRESHOLD_SHIFT:TOUCHPAD_PRESS_THRESHOLD))
			{
				if(touchpad_released[pad]==TOUCHPAD_RELEASE_TIMEOUT)
				{
					touchpad_state[pad] = 1;
					touchpad_released[pad] = 0;

					/*
					if(!bottom_key_pressed || !IS_BOTTOM_KEY(pad))
					{
						touchpad_event[pad] = 1;

						touchpad_state[pad] = 1;
						//printf("touch_pad_scan(): touchpad_state[%d] => 1\n", pad);
						touchpad_released[pad] = 0;

						if(IS_BOTTOM_KEY(pad))
						{
							bottom_key_pressed = 1;
							printf("touch_pad_scan(): bottom key pressed, pad = %d\n", pad);
						}
					}
					else*/ if(!bottom_key_pressed && IS_BOTTOM_KEY(pad))
					{
						bottom_key_pressed = 1;
						//printf("touch_pad_scan(): bottom key pressed, pad = %d\n", pad);
						touchpad_event[pad] = 1;
						last_ui_event = UI_EVENT_KEY;
					}
					else if(IS_SET_KEY(pad) && !set_pressed)
					{
						set_pressed = 1;
						#ifdef DEBUG_OUTPUT
						printf("touch_pad_scan(): SET pressed\n");
						#endif
						touchpad_event[pad] = 1;
						last_ui_event = UI_EVENT_SET;
					}
					else if(IS_SHIFT_KEY(pad) && !shift_pressed)
					{
						shift_pressed = 1;
						#ifdef DEBUG_OUTPUT
						printf("touch_pad_scan(): SHIFT pressed\n");
						#endif
						touchpad_event[pad] = 1;
						last_ui_event = UI_EVENT_SHIFT;
					}
				}
				/*
				else if(bottom_key_pressed)
				{
					//printf("touch_pad_scan(): bottom_key_pressed already = 1\n");
				}
				*/
			}
			else
			{
				if(touchpad_released[pad]<TOUCHPAD_RELEASE_TIMEOUT)
				{
					touchpad_released[pad]++;
				}
				else
				{
					if(touchpad_state[pad])
					{
						touchpad_state[pad] = 0;
						//printf("touch_pad_scan(): touchpad_state[%d] => 0\n", pad);

						if(bottom_key_pressed && IS_BOTTOM_KEY(pad))
						{
							bottom_key_pressed = 0;
							//printf("touch_pad_scan(): bottom key released\n");
						}
						else if(IS_SET_KEY(pad) && set_pressed)
						{
							set_pressed = 0;
							set_held = 0;
							if(!shift_pressed) { shift_held = 0; } //to prevent locking after SET+encoder used
							#ifdef DEBUG_OUTPUT
							printf("touch_pad_scan(): SET released\n");
							#endif

							if(last_ui_event==UI_EVENT_SET) //nothing happened in between
							{
								set_clicked = 1;
								#ifdef DEBUG_OUTPUT
								printf("touch_pad_scan(): SET clicked\n");
								#endif
							}
						}
						else if(IS_SHIFT_KEY(pad) && shift_pressed)
						{
							shift_pressed = 0;
							shift_held = 0;
							if(!set_pressed) { set_held = 0; } //to prevent locking after SHIFT+encoder used
							#ifdef DEBUG_OUTPUT
							printf("touch_pad_scan(): SHIFT released\n");
							#endif

							if(last_ui_event==UI_EVENT_SHIFT) //nothing happened in between
							{
								shift_clicked = 1;
								#ifdef DEBUG_OUTPUT
								printf("touch_pad_scan(): SHIFT clicked\n");
								#endif
							}

							if(enc_function==ENC_FUNCTION_ADJUST_TEMPO)
							{
								enc_function = ENC_FUNCTION_NONE;
								led_indication_refresh = LED_INDICATION_REFRESH_TIMEOUT - 1;
							}
						}
					}
				}
			}
		}

		if(set_pressed || shift_pressed)
		{
			//printf("touch_pad_scan(): set_pressed = %d, shift_pressed = %d, set_held = %d, shift_held = %d\n",set_pressed,shift_pressed,set_held,shift_held);

			if(set_pressed && set_held>=0)
			{
				//printf("[%d\n",set_held);
				set_held++;
				if(set_held==SET_HELD_TIMEOUT && !shift_pressed)
				{
					set_held = -1;
					touchpad_event_SET_HELD = 1;
				}
			}

			if(shift_pressed && shift_held>=0)
			{
				//printf("%d]\n",shift_held);
				shift_held++;
				if(shift_held==SHIFT_HELD_TIMEOUT && !set_pressed)
				{
					shift_held = -1;
					touchpad_event_SHIFT_HELD = 1;
				}
			}

			if(set_held>=SET_HELD_TIMEOUT && shift_held>=SHIFT_HELD_TIMEOUT)
			{
				set_held = -1;
				shift_held = -1;
				touchpad_event_BOTH_HELD = 1;
			}

			if(encoder_changed)
			{
				encoder_changed = 0;
				set_held = -1;
				shift_held = -1;
			}
		}

		Delay(1);
		//printf("\n");
	}
}

#ifdef TOUCHPAD_TEST
void touch_pad_test(void *pvParameters)
{
	esp_err_t res;
	uint16_t touch_value, touch_filter_value = 0;

	// Initialize touch pad peripheral.
	// The default fsm mode is software trigger mode.
	res = touch_pad_init();
	printf("touch_pad_test(): touch_pad_init returned code %d\n", res);

	// Set reference voltage for charging/discharging
	// In this case, the high reference voltage will be 2.7V - 1V = 1.7V
	// The low reference voltage will be 0.5
	// The larger the range, the larger the pulse count value.
	res = touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
	//res = touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_KEEP);
	//res = touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_0V);
	printf("touch_pad_test(): touch_pad_set_voltage returned code %d\n", res);

	res = touch_pad_config(TOUCH_PAD_NUM0, TOUCH_THRESH_NO_USE);
	//printf("touch_pad_test(): touch_pad_config returned code %d\n", res);
	res = touch_pad_config(TOUCH_PAD_NUM1, TOUCH_THRESH_NO_USE);
	//printf("touch_pad_test(): touch_pad_config returned code %d\n", res);
	res = touch_pad_config(TOUCH_PAD_NUM2, TOUCH_THRESH_NO_USE);
	//printf("touch_pad_test(): touch_pad_config returned code %d\n", res);
	res = touch_pad_config(TOUCH_PAD_NUM3, TOUCH_THRESH_NO_USE);
	//printf("touch_pad_test(): touch_pad_config returned code %d\n", res);
	res = touch_pad_config(TOUCH_PAD_NUM4, TOUCH_THRESH_NO_USE);
	//printf("touch_pad_test(): touch_pad_config returned code %d\n", res);
	res = touch_pad_config(TOUCH_PAD_NUM5, TOUCH_THRESH_NO_USE);
	//printf("touch_pad_test(): touch_pad_config returned code %d\n", res);
	res = touch_pad_config(TOUCH_PAD_NUM6, TOUCH_THRESH_NO_USE);
	//printf("touch_pad_test(): touch_pad_config returned code %d\n", res);
	res = touch_pad_config(TOUCH_PAD_NUM7, TOUCH_THRESH_NO_USE);
	//printf("touch_pad_test(): touch_pad_config returned code %d\n", res);
	res = touch_pad_config(TOUCH_PAD_NUM8, TOUCH_THRESH_NO_USE);
	//printf("touch_pad_test(): touch_pad_config returned code %d\n", res);
	res = touch_pad_config(TOUCH_PAD_NUM9, TOUCH_THRESH_NO_USE);
	//printf("touch_pad_test(): touch_pad_config returned code %d\n", res);

	/*
	for (int i = 0;i< TOUCH_PAD_MAX;i++)
	{
		touch_pad_config(i, TOUCH_THRESH_NO_USE);
	}
	*/

	//res = touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
	//printf("touch_pad_test(): touch_pad_filter_start returned code %d\n", res);

	while(1)
	{
		//res = touch_pad_read(TOUCH_PAD_NUM7, &touch_value);
		//printf("touch_pad_read(): touch_pad_read returned code %d, value = %d\n", res, touch_value);

		touch_pad_read(TOUCH_PAD_NUM0, &touch_value);
		//touch_pad_read_filtered(TOUCH_PAD_NUM0, &touch_filter_value);
		printf("%d:[%4d,%4d]\t", TOUCH_PAD_NUM0, touch_value, touch_filter_value);

		#ifdef TOUCHPAD_LED_TEST
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { LED_0_ON; } else { LED_0_OFF; }
		#endif

		touch_pad_read(TOUCH_PAD_NUM1, &touch_value);
		//touch_pad_read_filtered(TOUCH_PAD_NUM1, &touch_filter_value);
		printf("%d:[%4d,%4d]\t", TOUCH_PAD_NUM1, touch_value, touch_filter_value);

		#ifdef TOUCHPAD_LED_TEST
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { LED_1_ON; } else { LED_1_OFF; }
		#endif

		touch_pad_read(TOUCH_PAD_NUM2, &touch_value);
		//touch_pad_read_filtered(TOUCH_PAD_NUM2, &touch_filter_value);
		printf("%d:[%4d,%4d]\t", TOUCH_PAD_NUM2, touch_value, touch_filter_value);

		#ifdef TOUCHPAD_LED_TEST
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { LED_2_ON; } else { LED_2_OFF; }
		#endif

		touch_pad_read(TOUCH_PAD_NUM3, &touch_value);
		//touch_pad_read_filtered(TOUCH_PAD_NUM3, &touch_filter_value);
		printf("%d:[%4d,%4d]\t", TOUCH_PAD_NUM3, touch_value, touch_filter_value);

		#ifdef TOUCHPAD_LED_TEST
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { LED_3_ON; } else { LED_3_OFF; }
		#endif

		touch_pad_read(TOUCH_PAD_NUM4, &touch_value);
		//touch_pad_read_filtered(TOUCH_PAD_NUM4, &touch_filter_value);
		printf("%d:[%4d,%4d]\t", TOUCH_PAD_NUM4, touch_value, touch_filter_value);

		#ifdef TOUCHPAD_LED_TEST
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { LED_4_ON; } else { LED_4_OFF; }
		#endif

		touch_pad_read(TOUCH_PAD_NUM5, &touch_value);
		//touch_pad_read_filtered(TOUCH_PAD_NUM5, &touch_filter_value);
		printf("%d:[%4d,%4d]\t", TOUCH_PAD_NUM5, touch_value, touch_filter_value);

		#ifdef TOUCHPAD_LED_TEST
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { LED_5_ON; } else { LED_5_OFF; }
		#endif

		touch_pad_read(TOUCH_PAD_NUM6, &touch_value);
		//touch_pad_read_filtered(TOUCH_PAD_NUM6, &touch_filter_value);
		printf("%d:[%4d,%4d]\t", TOUCH_PAD_NUM6, touch_value, touch_filter_value);

		#ifdef TOUCHPAD_LED_TEST
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { LED_6_ON; } else { LED_6_OFF; }
		#endif

		touch_pad_read(TOUCH_PAD_NUM7, &touch_value);
		//touch_pad_read_filtered(TOUCH_PAD_NUM7, &touch_filter_value);
		printf("%d:[%4d,%4d]\t", TOUCH_PAD_NUM7, touch_value, touch_filter_value);

		#ifdef TOUCHPAD_LED_TEST
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { LED_7_ON; } else { LED_7_OFF; }
		#endif

		touch_pad_read(TOUCH_PAD_NUM8, &touch_value);
		//touch_pad_read_filtered(TOUCH_PAD_NUM8, &touch_filter_value);
		printf("%d:[%4d,%4d]\t", TOUCH_PAD_NUM8, touch_value, touch_filter_value);

		#ifdef TOUCHPAD_LED_TEST
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { LED_8_ON; } else { LED_8_OFF; }
		#endif

		touch_pad_read(TOUCH_PAD_NUM9, &touch_value);
		//touch_pad_read_filtered(TOUCH_PAD_NUM9, &touch_filter_value);
		printf("%d:[%4d,%4d]\n", TOUCH_PAD_NUM9, touch_value, touch_filter_value);

		#ifdef TOUCHPAD_LED_TEST
		if(touch_value<TOUCHPAD_PRESS_THRESHOLD) { LED_9_ON; } else { LED_9_OFF; }
		#endif

		/*
		for (int i = 0; i < TOUCH_PAD_MAX; i++)
		{
			 touch_pad_read_filtered(i, &touch_filter_value);
			 touch_pad_read(i, &touch_value);
			 printf("T%d:[%4d,%4d] ", i, touch_value, touch_filter_value);
		}
		printf("\n");
		*/

		Delay(2);
	}
}
#endif

void indicate_sensors_base_range()
{
	int8_t base,range;

	//range: 1-5-20-40-1000
	if(sensor_range<SENSOR_RANGE_DEFAULT/4)
	{
		range = 4;
	}
	else if(sensor_range<SENSOR_RANGE_DEFAULT)
	{
		range = 3;
	}
	else if(sensor_range<SENSOR_RANGE_DEFAULT*2)
	{
		range = 2;
	}
	else
	{
		range = 1;
	}

	//range: 1-50-100-500-10000
	if(sensor_base<SENSOR_BASE_DEFAULT/2)
	{
		base = 4;
	}
	else if(sensor_base<SENSOR_BASE_DEFAULT)
	{
		base = 3;
	}
	else if(sensor_base<SENSOR_BASE_DEFAULT*5)
	{
		base = 2;
	}
	else
	{
		base = 1;
	}

	LEDS_ORANGE_GLOW2(base,range);
}

spi_flash_mmap_handle_t mmap_handle_samples;
void *samples_ptr1 = NULL;

void flash_map_samples()
{
	if(samples_ptr1!=NULL)
	{
		#ifdef DEBUG_OUTPUT
		printf("flash_map_samples(): samples flash already mapped: samples_ptr1 = 0x%x\n", (unsigned int)samples_ptr1);
		#endif
		return;
	}

	//mapping address must be aligned to 64kB blocks (0x10000)
	int sample_buffer_adr = SAMPLES_BASE;
	#ifdef DEBUG_OUTPUT
	printf("flash_map_samples(): mapping sample at address %x\n", sample_buffer_adr);
	#endif

	int sample_buffer_adr_aligned_64k = sample_buffer_adr & 0xFFFF0000;
	//sample_buffer_align_offset = mixed_sample_buffer_adr - mixed_sample_buffer_adr_aligned_64k;

	int sample_buffer_align_offset = 0;
	int sample_region_size = SAMPLES_LENGTH;

	#ifdef DEBUG_OUTPUT
	printf("flash_map_samples(): mapping address aligned to %x (%d), offset = %x (%d)\n", sample_buffer_adr_aligned_64k, sample_buffer_adr_aligned_64k, sample_buffer_align_offset, sample_buffer_align_offset);
	//printf("Mapping flash memory at %x (+%x length)\n", mixed_sample_buffer_adr_aligned_64k, mixed_sample_buffer_align_offset + MIXED_SAMPLE_BUFFER_LENGTH*2);
	#endif

	int esp_result = spi_flash_mmap(sample_buffer_adr_aligned_64k,
									(size_t)(sample_buffer_align_offset+sample_region_size),
									SPI_FLASH_MMAP_DATA,
									&samples_ptr1,
									&mmap_handle_samples);

	#ifdef DEBUG_OUTPUT
	if(esp_result!=ESP_OK)
	{
		printf("flash_map_samples(): spi_flash_mmap() result error code = %d (0x%x)\n", esp_result, esp_result);
	}
	else
	{
		printf("flash_map_samples(): spi_flash_mmap() returned ESP_OK\n");
	}

	printf("flash_map_samples(): spi_flash_mmap() mapped destination = 0x%x\n", (unsigned int)samples_ptr1);
	#endif

	if(samples_ptr1==NULL)
	{
		printf("flash_map_samples(): spi_flash_mmap() returned NULL pointer\n");
		while(1);
	}

	#ifdef DEBUG_OUTPUT
	printf("flash_map_samples(): spi_flash_mmap() result: handle=%d ptr=%p\n", mmap_handle_samples, samples_ptr1);
	#endif
}

void show_firmware_version()
{
	LEDS_display_direct(FW_VERSION_INDICATOR_ORANGE,FW_VERSION_INDICATOR_BLUE); //orange showing version, blue showing sub-version
	Delay(5000);
	LEDS_BLUE_OFF;
	LEDS_display_direct_end();
}

void show_mac()
{
	uint8_t fm[8];
	esp_efuse_mac_get_default(fm);
	while(1)
	{
		for(int i=0;i<6;i++)
		{
			LEDS_display_direct(fm[i],0x01<<i); //orange showing byte, blue showing byte number
			Delay(2000);
		}
	}
	//LEDS_BLUE_OFF;
	//LEDS_display_direct_end();
}

void run_self_test()
{
	printf("run_self_test()\n");

	uint8_t orange=0xff, blue=0xff;

	int sensors_test[4] = {0,0,0,0};

	while(orange | blue)
	{
		LEDS_display_direct(orange, blue);

		for(int i=0;i<8;i++)
		{
			if(touchpad_event[i]==1)
			{
				blue &= ~(1<<i);
			}
		}
		for(int i=8;i<10;i++)
		{
			if(touchpad_event[i]==1)
			{
				orange &= ~(1<<(i-8));
			}
		}
		if(encoder_results[ENCODER_RIGHT]<0) { orange &= ~(0x80); }
		if(encoder_results[ENCODER_RIGHT]>0) { orange &= ~(0x40); }
		if(encoder_results[ENCODER_LEFT]<0) { orange &= ~(0x20); }
		if(encoder_results[ENCODER_LEFT]>0) { orange &= ~(0x10); }

		//printf("s=%f,%f\n", light_sensor_results[0], light_sensor_results[1]);
		if(light_sensor_results[0]>2000) sensors_test[0] = 1;
		if(light_sensor_results[0]<100) sensors_test[1] = 1;

		if(light_sensor_results[1]>2000) sensors_test[2] = 1;
		if(light_sensor_results[1]<100) sensors_test[3] = 1;

		if(sensors_test[0] && sensors_test[1]) { orange &= ~(0x08); }
		if(sensors_test[2] && sensors_test[3]) { orange &= ~(0x04); }

		Delay(10);
	}

	printf("run_self_test(): passed\n");
	store_selftest_pass(1);

	Delay(200);
	MCU_restart();
}

void light_sensors_test()
{
	printf("light_sensors_test()\n");

	uint8_t orange, blue;

	Delay(200);
	LEDS_display_direct_end();
	LEDS_BLUE_OFF;
	Delay(200);

	shift_pressed = 0;
	set_pressed = 0;

	while(!shift_pressed && !set_pressed)
	{
		printf("GPI34,35 values:	%f	%f\n", light_sensor_results[0], light_sensor_results[1]);

		if(light_sensor_results[0]>4000) { blue = 0xff; }
		else if(light_sensor_results[0]>3500) { blue = 0xfe; }
		else if(light_sensor_results[0]>3000) { blue = 0xfc; }
		else if(light_sensor_results[0]>2000) { blue = 0xf8; }
		else if(light_sensor_results[0]>1000) { blue = 0xf0; }
		else if(light_sensor_results[0]>500) { blue = 0xe0; }
		else if(light_sensor_results[0]>200) { blue = 0xc0; }
		else if(light_sensor_results[0]>100) { blue = 0x80; }
		else { blue = 0x00; }

		if(light_sensor_results[1]>4000) { orange = 0xff; }
		else if(light_sensor_results[1]>3500) { orange = 0x7f; }
		else if(light_sensor_results[1]>3000) { orange = 0x3f; }
		else if(light_sensor_results[1]>2000) { orange = 0x1f; }
		else if(light_sensor_results[1]>1000) { orange = 0x0f; }
		else if(light_sensor_results[1]>500) { orange = 0x07; }
		else if(light_sensor_results[1]>200) { orange = 0x03; }
		else if(light_sensor_results[1]>100) { orange = 0x01; }
		else { orange = 0x00; }

		LEDS_display_direct(orange,blue);
		Delay(20);
	}
	printf("light_sensors_test(): end\n");
}
