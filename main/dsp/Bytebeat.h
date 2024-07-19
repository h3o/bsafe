/*
 * Bytebeat.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 14 Jul 2018
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

#ifndef EXTENSIONS_BYTEBEAT_H_
#define EXTENSIONS_BYTEBEAT_H_

#include <stdint.h> //for uint16_t type

#define NUM_PATCHES 8

#ifdef SAMPLING_RATE_32KHZ
#define BYTEBEAT_TEMPO_CORRECTION	44 //45
#endif

#define BYTEBEAT_SONGS 				8
#define BYTEBEAT_LENGTH_UNLIMITED 	-1
#define BYTEBEAT_LENGTH_DEFAULT 	(I2S_AUDIOFREQ) //32000
#define BYTEBEAT_LENGTH_MIN			64
#define BYTEBEAT_LENGTH_MAX			(I2S_AUDIOFREQ*64)

#define BYTEBEAT_START_STEP_FAST	(I2S_AUDIOFREQ/25) //1280
#define BYTEBEAT_START_STEP_NORMAL	(I2S_AUDIOFREQ/100) //320
#define BYTEBEAT_START_STEP_SLOW	(I2S_AUDIOFREQ/400) //80

#define BYTEBEAT_LENGTH_STEP		(I2S_AUDIOFREQ/1000) //32

#define BYTEBEAT_LENGTH_THR_FINE	200
#define BYTEBEAT_LENGTH_STEP_FINE	(I2S_AUDIOFREQ/500) //64

#define BYTEBEAT_LENGTH_THR_COARSE	6000
#define BYTEBEAT_LENGTH_STEP_COARSE (I2S_AUDIOFREQ/2000) //16

extern uint8_t bytebeat_echo_on;
extern uint8_t BB_SHIFT_VOLUME;
extern uint8_t BB_SHIFT_VOLUME_0;

#define SEQUENCER_TIMING_DEFAULT	(I2S_AUDIOFREQ/2) //(1024*16)
#define SEQUENCER_TIMING_MIN		64
extern int32_t SEQUENCER_TIMING;
extern const uint32_t tempo_table[];

extern int bytebeat_song_length_effective, bytebeat_song_length, bytebeat_song_ptr, bytebeat_song_start_effective, bytebeat_song_start;
extern uint16_t bytebeat_song_length_div, bytebeat_song_pos_shift;
extern int bb_ptr_continuous, bb_last_note_triggered;

extern int patch_song_start[], patch_song_length[];
extern int8_t bytebeat_song, patch, patch_song[];
extern uint8_t encoder_blink, sequencer_blink;
extern int32_t sequencer_counter;

extern uint8_t bit1, bit2;

extern int patch_var1[], patch_var2[], patch_var3[], patch_var4[], patch_bit1[], patch_bit2[];
#define	PATCH_SILENCE (-1)

extern unsigned char var_p[];
extern int variable_n, arpeggiator, arp_pattern[], arp_ptr, arp_active_steps, arp_rep_cnt, arp_seq_dir, arp_octave, arp_repeat, arp_range_from, arp_range_to, arp_octave_effective;
extern int seq_octave, seq_octave_mode, seq_range_from, seq_range_to, seq_octave_effective;
extern uint8_t arp_layer, seq_layer;

#define ARP_LAYER_MATCHING	(arp_layer==layer || (layer>=1 && layer<=WAVETABLE_LAYERS && arp_layer>=1 && arp_layer<=WAVETABLE_LAYERS))
#define SEQ_LAYER_MATCHING	(seq_layer==layer || (layer>=1 && layer<=WAVETABLE_LAYERS && seq_layer>=1 && seq_layer<=WAVETABLE_LAYERS))

#define ARP_LAYER_DIFFERENT	(arp_layer!=layer)
#define SEQ_LAYER_DIFFERENT	(seq_layer!=layer)

#define ARP_OCTAVE_RANGE_MIN		-2
#define ARP_OCTAVE_RANGE_MAX		12
#define ARP_REPEAT_MAX				8

#define SEQ_OCTAVE_RANGE_MIN		-2
#define SEQ_OCTAVE_RANGE_MAX		12
#define SEQ_OCTAVE_MODES_MAX		3

#define SEQ_OCTAVE_MODE_UP			0
#define SEQ_OCTAVE_MODE_DOWN		1
#define SEQ_OCTAVE_MODE_UPDOWN		2
#define SEQ_OCTAVE_MODE_RANDOM		3

#define NUM_SEQUENCES 		8
#define SEQ_STEPS_D_R1		4
#define SEQ_STEPS_D_R2		2
#define SEQ_STEPS_DEFAULT	(SEQ_STEPS_D_R1*SEQ_STEPS_D_R2)
#define SEQ_STEPS_MAX		64
extern int8_t sequencer, sequencer_steps[];
extern int8_t seq_steps_r1[], seq_steps_r2[];

#define SEQ_PATTERN_PAUSE		127 //value to indicate skipped beat (silence)
#define SEQ_PATTERN_CONTINUE	126 //value to indicate beat with patch continuing
extern int8_t* seq_pattern[];
extern int8_t seq_filled_steps[];
#define SEQUENCE_COMPLETE(s) (seq_filled_steps[s-1]>=sequencer_steps[s-1])
#define SEQ_STEP_START (-1)
extern int seq_step_ptr, seq_running;

#define ARP_MAX		8
#define ARP_NONE	0

#define ARP_UP		1
#define ARP_DOWN	2
#define ARP_UPDN	3
#define ARP_UPDN_L	4
//#define ARP_UP2		5
//#define ARP_DOWN2	6
//#define ARP_UPDN2	7
//#define ARP_ORDER	7
#define ARP_STEP	5
#define ARP_STEP_L	6
#define ARP_RANDOM	7
#define ARP_RND_L	8

#define ARP_MAX_STEPS	8

extern int layer, bank, last_loaded_patch, sound_stopped;
extern int layer7_playhead, layer7_grain_length, layer7_grain_bitrate, layer7_last_key;
extern int layer7_patch_modified, layer7_samples_delete;
extern int layer7_patch_start[];
extern int layer7_patch_length[];
extern int layer7_patch_bitrate[];

#define FLASH_SAMPLE_BITRATE_MIN		-24
#define FLASH_SAMPLE_BITRATE_MAX		8
#define FLASH_SAMPLE_BITRATE_DEFAULT	1

extern uint8_t layers_active;

#define LAYER_FLASH_SAMPLE					7
#define FLASH_SAMPLE_BOOST_VOLUME_MIN		-3
#define FLASH_SAMPLE_BOOST_VOLUME_MAX		4
#define FLASH_SAMPLE_BOOST_VOLUME_DEFAULT	0

#define IS_FLASH_SAMPLE_LAYER	(layer==LAYER_FLASH_SAMPLE)

extern int8_t FLASH_SAMPLE_BOOST_VOLUME;
extern int8_t FLASH_SAMPLE_FORMAT;

#define FLASH_SAMPLE_LENGTH_MIN				1000
#define FLASH_SAMPLE_LENGTH_DEFAULT			32000 //2 sec at default bitrate
#define FLASH_SAMPLE_RESIZING_PREVIEW		4000 //0.25 sec at default bitrate


#ifdef __cplusplus
extern "C" {
#endif

void bytebeat_engine();
int16_t bytebeat_echo(int16_t sample);

void bytebeat_init();
uint32_t bytebeat_next_sample();

//void bytebeat_next_song();
//void bytebeat_stereo_paning();

int is_sequence_allocated(int position);
void init_sequence(int position);
void deinit_sequence(int position);
void animate_sequencer();
void start_stop_sequencer(int sequencer);
void reset_arp_active_steps();
void assign_current_patch_params();
void layer7_trigger_sample(int note);

#ifdef __cplusplus
}
#endif

#endif /* EXTENSIONS_BYTEBEAT_H_ */
