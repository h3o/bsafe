/*
 * SineWaves.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 25 Feb 2021
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

#ifndef DSP_SINEWAVES_H_
#define DSP_SINEWAVES_H_

#include <stdint.h> //for uint16_t type

#define MINI_PIANO_KEYS			8

//#define SINETABLE_SIZE		32779 //50777 //32000
//#define W_STEP_DEFAULT		200

//#define WAVETABLE_SIZE_DOUBLE

#ifdef WAVETABLE_SIZE_DOUBLE
#define WAVETABLE_SIZE			7919*2
#else
#define WAVETABLE_SIZE			7919 //prime number
//#define WAVETABLE_SIZE			8000
//#define WAVETABLE_SIZE			8192
#endif

//#define W_STEP_DEFAULT 			50

//#define SINETABLE_SIZE		1553
//#define W_STEP_DEFAULT 		10

//#define SINETABLE_SIZE		257
//#define W_STEP_DEFAULT		2

#define SINETABLE_AMP			128//255//32767

#define SAWTABLE_AMP			128
#define SQUARETABLE_AMP			128

#define SAWTABLE_DC_OFFSET		64
#define SQUARETABLE_DC_OFFSET	64

#define ENVTABLE_AMP			2.0f//100//200//65000//4000
#define ENVTABLE_OFFSET			0 //.04f//0//1500

/*
#define ENVTABLE_SIZE			1024
#define ENV_DECAY_FACTOR		0.2f
#define ENV_DECAY_STEP_DIV		20
*/

#define ENVTABLE_SIZE			500
#define ENV_DECAY_FACTOR		0.5f
#define ENV_DECAY_STEP_DIV		40

#define WT_CYCLE_ALIGNMENT_DEFAULT		0x01 //left side (layers 2-4) aligned, right side (layers 5-7) not aligned

extern int note_triggered, last_note_triggered, wt_octave_shift, wt_cycle_alignment;
extern uint16_t mini_piano_tuning[];
extern int32_t mini_piano_tuning_enc;
extern uint8_t mini_piano_waves[];
extern int8_t rnd_envelope_div[];

#define MINI_PIANO_TUNING_MIN_RNG		2000
#define MINI_PIANO_TUNING_MIN			5
#define MINI_PIANO_TUNING_MIN_OCTAVE	100 	//higher minimum to not detune from the key too much
#define MINI_PIANO_TUNING_MAX			65000
#define MINI_PIANO_TUNING_BASE			20

#define MINI_PIANO_WAVE_DEFAULT	0
#define MINI_PIANO_WAVE_SINE	1
#define MINI_PIANO_WAVE_SAW		2
#define MINI_PIANO_WAVE_SQUARE	3
#define MINI_PIANO_WAVE_MULTI	4
#define MINI_PIANO_WAVE_NOISE	5
#define MINI_PIANO_WAVE_RNG		6

#define MINI_PIANO_WAVE_FIRST	MINI_PIANO_WAVE_SINE
#define MINI_PIANO_WAVE_LAST	MINI_PIANO_WAVE_RNG

#define WAVETABLE_LAYERS 		6
#define IS_WAVETABLE_LAYER 		(layer>=1 && layer<=WAVETABLE_LAYERS)
#define WAVETABLE_LAYERS_MASK	0x7e //bit mask for active layers

#define MINI_PIANO_LAYER_RNG	MINI_PIANO_WAVE_RNG
#define WAVE_RNG_DIVIDER_MAX	8

#define WAVESAMPLE_BOOST_VOLUME_MIN		-3
#define WAVESAMPLE_BOOST_VOLUME_MAX		4
#define WAVESAMPLE_BOOST_VOLUME_DEFAULT	0
extern int8_t WAVESAMPLE_BOOST_VOLUME;

#ifdef __cplusplus
extern "C" {
#endif

void sine_waves_init();
void sine_waves_next_sample(uint16_t *out_sample1, uint16_t *out_sample2);
void sine_waves_stop_sound();
void sine_waves_deinit();

void reset_higher_layers(int reset_mini_piano, int reset_samples);

#ifdef __cplusplus
}
#endif

#endif /* DSP_SINEWAVES_H_ */
