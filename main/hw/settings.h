/*
 * settings.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 28 Feb 2021
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

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "hw/init.h"
#include "dsp/Bytebeat.h"
#include "dsp/SineWaves.h"

#define PATCHES_PER_BANK	8
#define PATCH_BANKS			8

/*
typedef struct
{
	int VOLUME;
	int8_t VOLUME_updated;

	int32_t TEMPO;
	int8_t TEMPO_updated;

	//double FINE_TUNING;
	//int8_t FINE_TUNING_updated;

	//int8_t TRANSPOSE;
	//int8_t TRANSPOSE_updated;

	int8_t SENSORS;
	int8_t SENSORS_updated;

	int update;

} persistent_settings_t;
*/

typedef struct
{
	int	patch_no;
	int patch_song_start[NUM_PATCHES];
	int patch_song_length[NUM_PATCHES];
	int patch_var1[NUM_PATCHES];
	int patch_var2[NUM_PATCHES];
	int patch_var3[NUM_PATCHES];
	int patch_var4[NUM_PATCHES];
	int patch_bit1[NUM_PATCHES];
	int patch_bit2[NUM_PATCHES];
	int8_t patch_song[NUM_PATCHES];
	uint8_t bit1,bit2;
	unsigned char var_p[4];
	int variable_n, arpeggiator, arp_pattern[ARP_MAX_STEPS+1], arp_active_steps;

	uint8_t layer;

	uint8_t wt_cycle_alignment;

	uint8_t bb_ptr_continuous;
	uint8_t reserved2;

	int32_t SEQUENCER_TIMING;
	uint8_t bytebeat_echo_on;
	int echo_dynamic_loop_current_step, echo_dynamic_loop_length;
	int sensors_active, sensors_settings_shift, sensors_settings_div;
	uint16_t bytebeat_song_length_div, bytebeat_song_pos_shift;

	int8_t sequencer, sequencer_steps[NUM_SEQUENCES];
	int8_t seq_steps_r1[NUM_SEQUENCES], seq_steps_r2[NUM_SEQUENCES];

	uint8_t arp_layer;
	uint8_t seq_layer;

	uint8_t sensors_wt_layers;

	int8_t FLASH_SAMPLE_BOOST_VOLUME;
	int8_t WAVESAMPLE_BOOST_VOLUME;

	uint8_t mini_piano_waves[MINI_PIANO_KEYS/2]; //packed structure

	uint8_t layers_active;

	//right sensor calibration
	int16_t sensor_base;
	float sensor_range;

	int8_t seq_octave, seq_octave_mode, seq_range_from, seq_range_to;
	int8_t arp_octave, arp_repeat, arp_range_from, arp_range_to;

	int8_t seq_filled_steps[NUM_SEQUENCES];

	uint16_t mini_piano_tuning[MINI_PIANO_KEYS];
	int8_t rnd_envelope_div[MINI_PIANO_KEYS];

} bytebeat_patch_t;

typedef struct
{
	int layer7_patch_start[NUM_PATCHES];
	int layer7_patch_length[NUM_PATCHES];
	int layer7_patch_bitrate[NUM_PATCHES];

} sample_patch_t;

//extern persistent_settings_t persistent_settings;
extern uint8_t saved_patches;

extern uint64_t service_seq;
extern int service_seq_cnt;

#ifdef __cplusplus
 extern "C" {
#endif

void store_patch_nvs(bytebeat_patch_t *b_patch, int position, const char *prefix, int bytes_to_store);
int load_patch_nvs(void *b_patch, int position, const char *prefix, int bytes_expected);

void store_current_patch(int position);
void load_current_patch(int position);

uint8_t find_saved_patches(int bank);

//void store_settings(persistent_settings_t *settings);
//void load_settings(persistent_settings_t *settings);

void settings_reset();
void settings_stats();

void delete_last_loaded_patch();
void delete_sequence(int sequence_id);
void backup_restore_patches(const char *src, const char *dst);

int self_tested();
void store_selftest_pass(int value);
void reset_selftest_pass();

void set_sample_format(int format);
int get_sample_format();
void set_encoder_steps_per_event(int steps);
int get_encoder_steps_per_event();
void set_bb_volume(int vol);
int get_bb_volume();

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H_ */
