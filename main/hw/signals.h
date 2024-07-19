/*
 * signals.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 27 Apr 2016
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

#ifndef SIGNALS_H_
#define SIGNALS_H_

#include <stdbool.h>
#include <stdint.h> //for uint16_t type
#include "board.h"
#include "init.h"

#define ECHO_BUFFER_STATIC //needs to be dynamic to support other engines, but that did not work due to ESP32 malloc limitation (largest free block not large enough)

#define TIMING_BY_SAMPLE_1_SEC	sampleCounter==I2S_AUDIOFREQ
																	//freq		cycle at Fs=50780
#define TIMING_EVERY_1_MS		sampleCounter%(I2S_AUDIOFREQ/1000)	//1000Hz	0-50
#define TIMING_EVERY_2_MS		sampleCounter%(I2S_AUDIOFREQ/500)	//200Hz		0-101
#define TIMING_EVERY_4_MS		sampleCounter%(I2S_AUDIOFREQ/250)	//250Hz		0-203
#define TIMING_EVERY_5_MS		sampleCounter%(I2S_AUDIOFREQ/200)	//200Hz		0-253
#define TIMING_EVERY_10_MS		sampleCounter%(I2S_AUDIOFREQ/100)	//100Hz		0-507
#define TIMING_EVERY_20_MS		sampleCounter%(I2S_AUDIOFREQ/50)	//50Hz		0-1015
#define TIMING_EVERY_25_MS		sampleCounter%(I2S_AUDIOFREQ/40)	//40Hz		0-1269
#define TIMING_EVERY_40_MS		sampleCounter%(I2S_AUDIOFREQ/25)	//25Hz		0-2031
#define TIMING_EVERY_50_MS		sampleCounter%(I2S_AUDIOFREQ/20)	//20Hz		0-2539
#define TIMING_EVERY_83_MS		sampleCounter%(I2S_AUDIOFREQ/12)	//12Hz		0-4231
#define TIMING_EVERY_100_MS		sampleCounter%(I2S_AUDIOFREQ/10)	//10Hz		0-5078
#define TIMING_EVERY_125_MS		sampleCounter%(I2S_AUDIOFREQ/8)		//8Hz		0-6347
#define TIMING_EVERY_166_MS		sampleCounter%(I2S_AUDIOFREQ/6)		//6Hz		0-8463
#define TIMING_EVERY_200_MS		sampleCounter%(I2S_AUDIOFREQ/5)		//5Hz		0-10156
#define TIMING_EVERY_250_MS		sampleCounter%(I2S_AUDIOFREQ/4)		//4Hz		0-12695
#define TIMING_EVERY_500_MS		sampleCounter%(I2S_AUDIOFREQ/2)		//2Hz		0-25390

#define NOISE_SEED 19.1919191919191919191919191919191919191919

extern uint32_t sample32;
extern uint32_t sampleCounter, sampleArpSeq;

extern float sample_mix;

extern uint32_t random_value;

#ifdef __cplusplus
 extern "C" {
#endif

void reset_pseudo_random_seed();
void set_pseudo_random_seed(double new_value);

float PseudoRNG1a_next_float();
//float PseudoRNG1b_next_float();
//uint32_t PseudoRNG2_next_int32();

void new_random_value();
int fill_with_random_value(char *buffer);
void PseudoRNG_next_value(uint32_t *buffer);

//-------------------------- sample and echo buffers -------------------

#define AUDIOFREQ I2S_AUDIOFREQ //22050

#define ECHO_DYNAMIC_LOOP_STEPS 8//15
#define ECHO_DYNAMIC_LOOP_STEP_OFF 7

//the length is now controlled dynamically (as defined in signals.c)

#define ECHO_BUFFER_LENGTH (AUDIOFREQ * 3 / 2)	//48000 samples at 32000 rate, or 76170 samples, 152340 bytes, 1.5 sec at 48000 rate

#define ECHO_DYNAMIC_LOOP_LENGTH_DEFAULT_STEP 	0	//refers to echo_dynamic_loop_steps in signals.c
#define ECHO_DYNAMIC_LOOP_LENGTH_ECHO_OFF	 	(ECHO_DYNAMIC_LOOP_STEPS-1)	//corresponds to what is in echo_dynamic_loop_steps structure as 0

#define ECHO_DYNAMIC_LOOP_LENGTH_MAX	ECHO_BUFFER_LENGTH
#define ECHO_DYNAMIC_LOOP_LENGTH_MIN	2 //8 //(AUDIOFREQ / 256)

#ifdef ECHO_BUFFER_STATIC
extern int16_t echo_buffer[];	//the buffer is allocated statically
#else
extern int16_t *echo_buffer;	//the buffer is allocated dynamically
#endif

extern int echo_buffer_ptr0, echo_buffer_ptr;		//pointers for echo buffer
extern int echo_dynamic_loop_length;
extern int echo_dynamic_loop_current_step;
extern float echo_mix_f;
extern const int echo_dynamic_loop_steps[ECHO_DYNAMIC_LOOP_STEPS];

extern float SAMPLE_VOLUME;
#define SAMPLE_VOLUME_DEFAULT 9.0f

#define COMPUTED_SAMPLE_MIXING_LIMIT_UPPER 32000.0f
#define COMPUTED_SAMPLE_MIXING_LIMIT_LOWER -32000.0f

#define DYNAMIC_LIMITER_COEFF_DEFAULT 	1.0f
extern float limiter_coeff;

//---------------------------------------------------------------------------

void init_echo_buffer();
void deinit_echo_buffer();

#ifdef __cplusplus
}
#endif

#endif /* SIGNALS_H_ */
