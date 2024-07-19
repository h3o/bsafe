/*
 * init.h
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

#ifndef INIT_H_
#define INIT_H_

//#include <stdint.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
//#include "esp32/include/esp_event.h"
#include "esp_event_legacy.h"
#include "esp_attr.h"
#include "esp_sleep.h"
//#include "esp_event/include/esp_event_loop.h"
#include "esp_task_wdt.h"
#include "bootloader_random.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/mcpwm.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "esp_spi_flash.h"
#include "driver/uart.h"
#include "esp_heap_caps_init.h"
#include "driver/dac.h"
#include "driver/i2s.h"
#include "esp32/rom/rtc.h"
#include "soc\rtc_cntl_reg.h"
#include "driver\rtc_cntl.h"
//#include "esp32\include\esp_brownout.h"
#include "esp_intr_alloc.h"
//#include "soc\sens_reg.h"
//#include "soc\rtc_io_reg.h"
//#include "driver\touch_sensor_common.h"
#include "driver\touch_sensor.h"

#include "board.h"

#define TWDT_TIMEOUT_S						5
#define TASK_RESET_PERIOD_S					4

//#define SAMPLING_RATE_32KHZ

#ifdef SAMPLING_RATE_32KHZ
#define I2S_AUDIOFREQ	32000	//I (572) I2S: PLL_D2: Req RATE: 32000, real rate: 2032.000, BITS: 16, CLKM: 41, BCK: 60, MCLK: 41.667, SCLK: 65024.000000, diva: 64, divb: 42
#else
#define I2S_AUDIOFREQ	32768	//I (573) I2S: PLL_D2: Req RATE: 32768, real rate: 2083.000, BITS: 16, CLKM: 40, BCK: 60, MCLK: 40.690, SCLK: 66656.000000, diva: 64, divb: 44
#endif

#define I2S_NUM			I2S_NUM_0

#define SENSORS_ACTIVE_BOTH		3
#define SENSORS_ACTIVE_RIGHT	2
#define SENSORS_ACTIVE_LEFT		1
#define SENSORS_ACTIVE_NONE		0
extern int sensors_active, sensors_settings_shift, sensors_settings_div;

extern uint32_t sample32;
extern size_t i2s_bytes_rw;

//#define ENCODERS_ANALOG
//#define ENCODERS_TEST
#define ENCODER_EVENT_DEC		-1
#define ENCODER_EVENT_INC		1

#define ENCODER_LEFT			0
#define ENCODER_RIGHT			1

#define ENCODER_STEPS_PER_EVENT_DEFAULT 2
extern int8_t ENCODER_STEPS_PER_EVENT;

/*
#define ENCODER_EVENT_LEFT_LEFT		1
#define ENCODER_EVENT_LEFT_RIGHT	2
#define ENCODER_EVENT_RIGHT_LEFT	4
#define ENCODER_EVENT_RIGHT_RIGHT	8
*/

#define SENSORS_FLOAT

extern int8_t encoder_results[2];//, encoder_event[2];

#ifdef SENSORS_FLOAT
extern float light_sensor_results[2];
#define SENSOR_RANGE	sensor_range
#else
extern int16_t light_sensor_results[2];
#define SENSOR_RANGE	((int)sensor_range)
#endif

#define LIGHT_SENSOR_LPF_ALPHA	0.2f

//right sensor calibration
#define SENSOR_RANGE_MIN		1
#define SENSOR_RANGE_MAX		1000
#define SENSOR_RANGE_DEFAULT	20
#define SENSOR_BASE_MIN			1
#define SENSOR_BASE_MAX			10000
#define SENSOR_BASE_DEFAULT		100
extern float sensor_range;
extern int32_t sensor_base;
extern uint8_t sensors_wt_layers;

extern uint8_t led_disp[7];
#define LEDS_BLUE1 			0
#define LEDS_BLUE2			3
#define LEDS_ORANGE1		2
#define LEDS_ORANGE2		1
#define LEDS_ORANGE			4
#define LEDS_DIRECT_BLUE	5
#define LEDS_DIRECT_ORANGE	6
#define LEDS_BLINK			0x10
#define LEDS_BLINK			0x10

#define LED_BLINK_DELAY	40

#define LEDS_BLUE_OFF {led_disp[LEDS_BLUE1] = 0; led_disp[LEDS_BLUE2] = 0;}
//these macros will reset other side LED current status
#define LEDS_BLUE_BLINK(x) {if(x<5){led_disp[LEDS_BLUE1]=(x)|LEDS_BLINK;led_disp[LEDS_BLUE2]=0;}else{led_disp[LEDS_BLUE1]=0;led_disp[LEDS_BLUE2]=(x-4)|LEDS_BLINK;}}
#define LEDS_BLUE_GLOW(x) {if(x<5){led_disp[LEDS_BLUE1]=x;led_disp[LEDS_BLUE2]=0;}else{led_disp[LEDS_BLUE1]=0;led_disp[LEDS_BLUE2]=(x-4);}}
#define LEDS_BLUE_GLOW2(x,y) {led_disp[LEDS_BLUE1]=x;led_disp[LEDS_BLUE2]=y;}
#define LEDS_BLUE_GLOW2(x,y) {led_disp[LEDS_BLUE1]=x;led_disp[LEDS_BLUE2]=y;}
#define LEDS_ORANGE_GLOW2(x,y) {led_disp[LEDS_ORANGE1]=x;led_disp[LEDS_ORANGE2]=y;}
//these macros won't reset other side LED current status
#define LEDS_BLUE_GLOW12(x) {if(x<5){led_disp[LEDS_BLUE1]=x;}else{led_disp[LEDS_BLUE2]=(x-4);}}
#define LEDS_BLUE_BLINK12(x) {if(x<5){led_disp[LEDS_BLUE1]=x|LEDS_BLINK;}else{led_disp[LEDS_BLUE2]=(x-4)|LEDS_BLINK;}}

extern int led_indication_refresh;
#define LED_INDICATION_REFRESH_TIMEOUT	AUDIOFREQ*1 //1 sec

#define LED_DIRECT_DELAY	1

void LEDS_display_direct(uint8_t bitmap_orange, uint8_t bitmap_blue);
void LEDS_display_direct_blink(int blink_orange, int blink_blue);
void LEDS_display_direct_end();
void LEDS_display_load_save_animation(int position, int speed, int delays);

extern int enc_function;
extern int enc_tempo_mult;
#define ENC_FUNCTION_NONE			0
#define ENC_FUNCTION_ADJUST_TEMPO	1

extern int menu_function;
#define MENU_PLAY	0
#define MENU_LYR	1
#define MENU_FRM	2
#define MENU_SEQ	3
#define MENU_ARP	4
#define MENU_BIT	5
#define MENU_VAR	6
#define MENU_DEL	7
#define MENU_VOL	8

#define MENU_SETTINGS	13

#define MENU_EXIT	0

#define IS_MENU_FUNCTION			(menu_function>=MENU_LYR && menu_function<=MENU_VOL)
//#define IS_MENU_PLAY_OR_SETTINGS	(menu_function==MENU_PLAY || menu_function==MENU_SETTINGS)

extern int led_display_direct;

#define BB_VOLUME_DEFAULT	3
#define BB_VOLUME_MIN		0
#define BB_VOLUME_MAX		7

//#define TOUCHPAD_TEST
//#define TOUCHPAD_LED_TEST
#define TOUCHPAD_PRESS_THRESHOLD		1250
#define TOUCHPAD_PRESS_THRESHOLD_SHIFT	850

#define TOUCH_PAD_LEFT		9	//set
#define TOUCH_PAD_RIGHT		1	//shift
#define TOUCH_PAD_K1		8
#define TOUCH_PAD_K2		7
#define TOUCH_PAD_K3		6
#define TOUCH_PAD_K4		5
#define TOUCH_PAD_K5		4
#define TOUCH_PAD_K6		3
#define TOUCH_PAD_K7		2
#define TOUCH_PAD_K8		0

extern int touchpad_state[];
extern int touchpad_event[];

#define IS_BOTTOM_KEY(k) (k!=TOUCH_PAD_LEFT&&k!=TOUCH_PAD_RIGHT)
#define IS_SET_KEY(k) (k==TOUCH_PAD_LEFT)
#define IS_SHIFT_KEY(k) (k==TOUCH_PAD_RIGHT)

extern int bottom_key_pressed, set_pressed, shift_pressed, set_held, shift_held, shift_clicked, set_clicked;
extern int touchpad_event_SET_HELD, touchpad_event_SHIFT_HELD, touchpad_event_BOTH_HELD;
extern int encoder_changed;

#define KEYS_RECENTLY		1
#define ENCODERS_RECENTLY	2
extern int keys_or_encoders_recently;

#define UI_EVENT_NONE		0
#define UI_EVENT_SHIFT		1
#define UI_EVENT_SET		2
#define UI_EVENT_KEY		3
#define UI_EVENT_ENCODER	4
#define UI_EVENT_EXIT_MENU	5 //to prevent counting click after shift used for something else
extern int last_ui_event;

//anodes
#define LED_A0	GPIO_NUM_5
#define LED_A1	GPIO_NUM_9
#define LED_A2	GPIO_NUM_10
#define LED_A3	GPIO_NUM_18

//cathodes
#define LED_C0	GPIO_NUM_19
#define LED_C1	GPIO_NUM_21
#define LED_C2	GPIO_NUM_22
#define LED_C3	GPIO_NUM_23

#define SOUND_OFF				0
#define SOUND_ON				1
#define SOUND_ON_WHITE_NOISE	2
extern int sound_enabled;

/*
 * Macro to check the outputs of TWDT functions and trigger an abort if an
 * incorrect code is returned.
 */
#define CHECK_ERROR_CODE(returned, expected) ({                        \
            if(returned != expected){                                  \
                printf("TWDT ERROR\n");                                \
                abort();                                               \
            }                                                          \
})

//-------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

void init_deinit_TWDT();

float micros();
uint32_t micros_i();
uint32_t millis();

void generate_random_seed();

unsigned char byte_bit_reverse(unsigned char b);
void LEDs_init(int core);
void LEDs_test();
void LEDs_driver_task(void *pvParameters);

void encoders_init(int core);
void light_sensors_init(int core);
void DAC_init();
void DAC_test();
void send_silence(int samples);

void MCU_restart();
//void brownout_init();

void touch_pad_test(void *pvParameters);
void touch_pad_scan(void *pvParameters);

void indicate_sensors_base_range();

#define SAMPLES_BASE	0x200000
#define SAMPLES_LENGTH	0x200000
extern void *samples_ptr1;

void flash_map_samples();

void show_firmware_version();
void show_mac();

void run_self_test();
void light_sensors_test();

#ifdef __cplusplus
}
#endif

#endif
