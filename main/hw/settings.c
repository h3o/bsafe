/*
 * settings.c
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

#include "settings.h"

#include "hw/gpio.h"
#include "hw/init.h"
#include "hw/signals.h"
#include <string.h>

//#define DEBUG_OUTPUT

//persistent_settings_t persistent_settings;
uint8_t saved_patches;

uint64_t service_seq;
int service_seq_cnt;

void delete_patch_nvs(int position, int delete_patch, int delete_samples, int delete_sequences);

void store_patch_nvs(bytebeat_patch_t *b_patch, int position, const char *prefix, int bytes_to_store)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("store_patch_nvs(): problem with nvs_open(), error = %d\n", res);
		return;
	}

	char key[20];
	//sprintf(key,"patch%d",position);
	sprintf(key,"%s%d",prefix,position);

	//int bytes_to_store = sizeof(bytebeat_patch_t);
	#ifdef DEBUG_OUTPUT
	printf("store_patch_nvs(): updating key \"%s\" with blob of %d bytes\n", key, bytes_to_store);
	#endif

	res = nvs_set_blob(handle, key, b_patch, bytes_to_store);
	if(res!=ESP_OK) //problem writing data
	{
		printf("store_patch_nvs(): problem with nvs_set_blob() while updating key \"%s\", error = %d\n", key, res);
		nvs_close(handle);
		return;
	}
	res = nvs_commit(handle);
	if(res!=ESP_OK) //problem writing data
	{
		printf("store_patch_nvs(): problem with nvs_commit() while updating key \"%s\", error = %d\n", key, res);
	}

	nvs_close(handle);
}

int load_patch_nvs(void *b_patch, int position, const char *prefix, int bytes_expected)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("load_patch_nvs(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	char key[20];
	//sprintf(key,"patch%d",position);
	sprintf(key,"%s%d",prefix,position);
	//int bytes_expected = sizeof(bytebeat_patch_t);
	#ifdef DEBUG_OUTPUT
	printf("load_patch_nvs(): reading key \"%s\" (a blob of %d bytes)\n", key, bytes_expected);
	#endif

	size_t bytes_loaded = -1;
	res = nvs_get_blob(handle, key, b_patch, &bytes_loaded);
	if(res!=ESP_OK) //problem reading data
	{
		#ifdef DEBUG_OUTPUT
		printf("load_patch_nvs(): problem with nvs_get_blob() while reading key \"%s\", error = %d\n", key, res);
		#endif
		nvs_close(handle);
		return 0;
	}
	nvs_close(handle);

	if(bytes_loaded != bytes_expected)
	{
		printf("load_patch_nvs(): unexpected amount of bytes loaded from key \"%s\": %d vs %d (sizeof(bytebeat_patch_t))\n", key, bytes_loaded, bytes_expected);
	}
	return bytes_loaded;
}

void store_sequences_nvs(int position)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("store_sequences_nvs(): problem with nvs_open(), error = %d\n", res);
		return;
	}

	char key[20];
	int nvs_updated = 0;

	for(int seq=0;seq<NUM_SEQUENCES;seq++)
	{
		if(seq_filled_steps[seq]>0)
		{
			if(seq_filled_steps[seq]>SEQ_STEPS_MAX)
			{
				#ifdef DEBUG_OUTPUT
				printf("store_sequences_nvs(): seq_filled_steps[%d]==%d > SEQ_STEPS_MAX! (corrected)\n", seq, seq_filled_steps[seq]);
				#endif
				seq_filled_steps[seq] = SEQ_STEPS_MAX;
			}

			sprintf(key,"seq%d_%d",position,seq);
			int bytes_to_store = seq_filled_steps[seq] * sizeof(int8_t); //int8_t* seq_pattern[NUM_SEQUENCES]
			#ifdef DEBUG_OUTPUT
			printf("store_sequences_nvs(): updating key \"%s\" with blob of %d bytes\n", key, bytes_to_store);
			#endif

			res = nvs_set_blob(handle, key, seq_pattern[seq], bytes_to_store);
			if(res!=ESP_OK) //problem writing data
			{
				printf("store_sequences_nvs(): problem with nvs_set_blob() while updating key \"%s\", error = %d\n", key, res);
				nvs_close(handle);
				return;
			}
			nvs_updated = 1;
		}
	}
	if(nvs_updated)
	{
		res = nvs_commit(handle);
		if(res!=ESP_OK) //problem writing data
		{
			printf("store_sequences_nvs(): problem with nvs_commit() while updating key \"%s\", error = %d\n", key, res);
		}
	}
	else
	{
		#ifdef DEBUG_OUTPUT
		printf("store_sequences_nvs(): patch at position %d does not have any sequences\n", position);
		#endif
	}

	nvs_close(handle);
}

void load_sequences_nvs(int position)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("load_sequences_nvs(): problem with nvs_open(), error = %d\n", res);
		return;
	}

	char key[20];

	for(int seq=0;seq<NUM_SEQUENCES;seq++)
	{
		if(seq_filled_steps[seq]>0)
		{
			#ifdef DEBUG_OUTPUT
			printf("load_sequences_nvs(): comparing seq_filled_steps[%d] > SEQ_STEPS_MAX: %d > %d\n", seq, seq_filled_steps[seq], SEQ_STEPS_MAX);
			#endif
			if(seq_filled_steps[seq]>SEQ_STEPS_MAX)
			{
				#ifdef DEBUG_OUTPUT
				printf("load_sequences_nvs(): seq_filled_steps[%d]==%d > SEQ_STEPS_MAX! (corrected)\n", seq, seq_filled_steps[seq]);
				#endif
				seq_filled_steps[seq] = SEQ_STEPS_MAX;
			}

			sprintf(key,"seq%d_%d",position,seq);
			int bytes_expected = seq_filled_steps[seq] * sizeof(int8_t); //int8_t* seq_pattern[NUM_SEQUENCES]
			#ifdef DEBUG_OUTPUT
			printf("load_sequences_nvs(): reading key \"%s\" (a blob of %d bytes)\n", key, bytes_expected);
			#endif

			init_sequence(seq);

			size_t bytes_loaded = -1;
			res = nvs_get_blob(handle, key, seq_pattern[seq], &bytes_loaded);
			if(res!=ESP_OK) //problem reading data
			{
				printf("load_sequences_nvs(): problem with nvs_get_blob() while reading key \"%s\", error = %d\n", key, res);
				//nvs_close(handle);
				//return;
				deinit_sequence(seq);
				seq_filled_steps[seq] = 0;
				sequencer_steps[seq] = SEQ_STEPS_DEFAULT;
				seq_steps_r1[seq] = SEQ_STEPS_D_R1;
				seq_steps_r2[seq] = SEQ_STEPS_D_R2;
			}
			else
			{
				seq_filled_steps[seq] = bytes_loaded;
				if(bytes_loaded != bytes_expected)
				{
					printf("load_sequences_nvs(): unexpected amount of bytes loaded from key \"%s\": %d vs %d (sizeof(bytebeat_patch_t))\n", key, bytes_loaded, bytes_expected);
					//return;
				}
			}
		}
		else
		{
			deinit_sequence(seq);
			seq_filled_steps[seq] = 0;
			sequencer_steps[seq] = SEQ_STEPS_DEFAULT;
			seq_steps_r1[seq] = SEQ_STEPS_D_R1;
			seq_steps_r2[seq] = SEQ_STEPS_D_R2;
		}
	}

	nvs_close(handle);
}

void store_current_patch(int position)
{
	bytebeat_patch_t *b_patch;
	#ifdef DEBUG_OUTPUT
	printf("store_current_patch(%d): allocating %d bytes for bytebeat_patch_t\n", position, sizeof(bytebeat_patch_t));
	#endif
	b_patch = (bytebeat_patch_t*)malloc(sizeof(bytebeat_patch_t));

	b_patch->patch_no = position;
	memcpy(b_patch->patch_song_start, patch_song_start, NUM_PATCHES*sizeof(int));
	memcpy(b_patch->patch_song_length, patch_song_length, NUM_PATCHES*sizeof(int));
	memcpy(b_patch->patch_var1, patch_var1, NUM_PATCHES*sizeof(int));
	memcpy(b_patch->patch_var2, patch_var2, NUM_PATCHES*sizeof(int));
	memcpy(b_patch->patch_var3, patch_var3, NUM_PATCHES*sizeof(int));
	memcpy(b_patch->patch_var4, patch_var4, NUM_PATCHES*sizeof(int));
	memcpy(b_patch->patch_bit1, patch_bit1, NUM_PATCHES*sizeof(int));
	memcpy(b_patch->patch_bit2, patch_bit2, NUM_PATCHES*sizeof(int));
	memcpy(b_patch->patch_song, patch_song, NUM_PATCHES*sizeof(int8_t)); //int8_t patch_song[NUM_PATCHES]

	for(int i=0;i<NUM_PATCHES;i++)
	{
		if(patch_song[i]<0 || patch_song[i]>=NUM_PATCHES)
		{
			#ifdef DEBUG_OUTPUT
			printf("store_current_patch(%d): corrupt data detected in patch_song[%d]: %d\n", position, i, patch_song[i]);
			#endif
		}
	}

	b_patch->bit1 = bit1;
	b_patch->bit2 = bit2;
	//printf("store_current_patch(%d): sizeof(var_p) = %d", position, sizeof(var_p));
	memcpy(b_patch->var_p, var_p, 4); //unsigned char var_p[4];

	b_patch->arpeggiator = arpeggiator;
	memcpy(b_patch->arp_pattern, arp_pattern, (ARP_MAX_STEPS+1)*sizeof(int)); //int arp_pattern[ARP_MAX_STEPS+1]
	b_patch->arp_active_steps = arp_active_steps;
	b_patch->arp_octave = arp_octave;
	b_patch->arp_repeat = arp_repeat;
	b_patch->arp_range_from = arp_range_from;
	b_patch->arp_range_to = arp_range_to;

	b_patch->layer = layer;

	b_patch->layers_active = layers_active;
	b_patch->arp_layer = arp_layer;
	b_patch->seq_layer = seq_layer;

	b_patch->SEQUENCER_TIMING = SEQUENCER_TIMING;
	b_patch->bytebeat_echo_on = bytebeat_echo_on;
	b_patch->echo_dynamic_loop_current_step = echo_dynamic_loop_current_step;
	b_patch->echo_dynamic_loop_length = echo_dynamic_loop_length;

	b_patch->sensors_active = sensors_active;
	b_patch->sensors_settings_shift = sensors_settings_shift;
	b_patch->sensors_settings_div = sensors_settings_div;
	b_patch->sensor_base = sensor_base;
	b_patch->sensor_range = sensor_range;
	b_patch->sensors_wt_layers = sensors_wt_layers;
	b_patch->wt_cycle_alignment = wt_cycle_alignment;
	#ifdef DEBUG_OUTPUT
	printf("store_current_patch(%d): sensors_active/sensors_settings_shift/sensors_settings_div = %d/%d/%d, base/range = %d/%f, sensors_wt_layers = %02x, wt_cycle_alignment = %02x\n", position, sensors_active, sensors_settings_shift, sensors_settings_div, sensor_base, sensor_range, sensors_wt_layers, wt_cycle_alignment);
	#endif

	b_patch->bytebeat_song_pos_shift = bytebeat_song_pos_shift;
	b_patch->bytebeat_song_length_div = bytebeat_song_length_div;
	b_patch->bb_ptr_continuous = bb_ptr_continuous;
	#ifdef DEBUG_OUTPUT
	printf("store_current_patch(%d): bytebeat_song_pos_shift/bytebeat_song_length_div = %d/%d, bb_ptr_continuous = %d\n", position, bytebeat_song_pos_shift, bytebeat_song_length_div, bb_ptr_continuous);
	#endif

	b_patch->sequencer = sequencer;
	memcpy(b_patch->sequencer_steps, sequencer_steps, (NUM_SEQUENCES)*sizeof(int8_t)); //int8_t sequencer_steps[NUM_SEQUENCES]
	memcpy(b_patch->seq_steps_r1, seq_steps_r1, (NUM_SEQUENCES)*sizeof(int8_t)); //int8_t seq_steps_r1[NUM_SEQUENCES]
	memcpy(b_patch->seq_steps_r2, seq_steps_r2, (NUM_SEQUENCES)*sizeof(int8_t)); //int8_t seq_steps_r2[NUM_SEQUENCES]
	//memcpy(b_patch->seq_pattern, seq_pattern, (NUM_SEQUENCES)*sizeof(int8_t*)); //int8_t* seq_pattern[NUM_SEQUENCES];
	memcpy(b_patch->seq_filled_steps, seq_filled_steps, (NUM_SEQUENCES)*sizeof(int8_t)); //int8_t seq_filled_steps[NUM_SEQUENCES]

	b_patch->seq_octave = seq_octave;
	b_patch->seq_octave_mode = seq_octave_mode;
	b_patch->seq_range_from = seq_range_from;
	b_patch->seq_range_to = seq_range_to;

	memcpy(b_patch->mini_piano_tuning, mini_piano_tuning, (MINI_PIANO_KEYS)*sizeof(uint16_t)); //uint16_t mini_piano_tuning[MINI_PIANO_KEYS]
	memcpy(b_patch->rnd_envelope_div, rnd_envelope_div, (MINI_PIANO_KEYS)*sizeof(int8_t)); //int8_t rnd_envelope_div[MINI_PIANO_KEYS]

	#ifdef DEBUG_OUTPUT
	printf("store_current_patch(%d): packing mini_piano_waves variable: ", position);
	#endif
	for(int i=0;i<MINI_PIANO_KEYS/2;i++)
	{
		//uint8_t mini_piano_waves[MINI_PIANO_KEYS]
		b_patch->mini_piano_waves[i] = mini_piano_waves[2*i] + 16*mini_piano_waves[2*i+1];
		#ifdef DEBUG_OUTPUT
		printf("(%x-%x) => %02x 	", mini_piano_waves[2*i], mini_piano_waves[2*i+1], b_patch->mini_piano_waves[i]);
		#endif
	}
	#ifdef DEBUG_OUTPUT
	printf("\n");
	#endif

	b_patch->WAVESAMPLE_BOOST_VOLUME = WAVESAMPLE_BOOST_VOLUME;
	b_patch->FLASH_SAMPLE_BOOST_VOLUME = FLASH_SAMPLE_BOOST_VOLUME;

	store_patch_nvs((void*)b_patch, position+bank*8, "patch", sizeof(bytebeat_patch_t));
	free(b_patch);

	if(layer7_patch_modified)
	{
		sample_patch_t *s_patch;
		s_patch = (sample_patch_t*)malloc(sizeof(sample_patch_t));
		memcpy(s_patch->layer7_patch_start, layer7_patch_start, NUM_PATCHES*sizeof(int)); //int layer7_patch_start[NUM_PATCHES];
		memcpy(s_patch->layer7_patch_length, layer7_patch_length, NUM_PATCHES*sizeof(int)); //int layer7_patch_length[NUM_PATCHES];
		memcpy(s_patch->layer7_patch_bitrate, layer7_patch_bitrate, NUM_PATCHES*sizeof(int)); //int layer7_patch_bitrate[NUM_PATCHES];
		store_patch_nvs((void*)s_patch, position+bank*8, "smp", sizeof(sample_patch_t));
		free(s_patch);
	}
	else if(layer7_samples_delete) //if scheduled to delete by service menu command
	{
		#ifdef DEBUG_OUTPUT
		printf("store_current_patch(%d): layer7_samples_delete = %d\n", position, layer7_samples_delete);
		#endif
		delete_patch_nvs(position+bank*8, 0, 1, 0); //delete samples in case they existed
	}

	//display sequencer stats
	#ifdef DEBUG_OUTPUT
	printf("store_current_patch(%d): sequencer_steps={", position);
	for(int i=0;i<NUM_SEQUENCES;i++){printf("%d,",sequencer_steps[i]);}
	printf("}\nstore_current_patch(%d): seq_steps_r1={", position);
	for(int i=0;i<NUM_SEQUENCES;i++){printf("%d,",seq_steps_r1[i]);}
	printf("}\nstore_current_patch(%d): seq_steps_r2={", position);
	for(int i=0;i<NUM_SEQUENCES;i++){printf("%d,",seq_steps_r2[i]);}
	printf("}\nstore_current_patch(%d): seq_filled_steps={", position);
	for(int i=0;i<NUM_SEQUENCES;i++){printf("%d,",seq_filled_steps[i]);}
	printf("}\n");
	#endif

	//store sequences
	store_sequences_nvs(position+bank*8);
}

void load_current_patch(int position)
{
	bytebeat_patch_t *b_patch;
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): allocating %d bytes for bytebeat_patch_t\n", position, sizeof(bytebeat_patch_t));
	#endif
	b_patch = (bytebeat_patch_t*)malloc(sizeof(bytebeat_patch_t));
	load_patch_nvs((void*)b_patch, position+bank*8, "patch", sizeof(bytebeat_patch_t));
	last_loaded_patch = position+bank*8;
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): patch loaded, stored position record = %d\n", position, b_patch->patch_no);
	#endif

	memcpy(patch_song_start, b_patch->patch_song_start, NUM_PATCHES*sizeof(int));
	memcpy(patch_song_length, b_patch->patch_song_length, NUM_PATCHES*sizeof(int));
	memcpy(patch_var1, b_patch->patch_var1, NUM_PATCHES*sizeof(int));
	memcpy(patch_var2, b_patch->patch_var2, NUM_PATCHES*sizeof(int));
	memcpy(patch_var3, b_patch->patch_var3, NUM_PATCHES*sizeof(int));
	memcpy(patch_var4, b_patch->patch_var4, NUM_PATCHES*sizeof(int));
	memcpy(patch_bit1, b_patch->patch_bit1, NUM_PATCHES*sizeof(int));
	memcpy(patch_bit2, b_patch->patch_bit2, NUM_PATCHES*sizeof(int));
	memcpy(patch_song, b_patch->patch_song, NUM_PATCHES*sizeof(int8_t)); //int8_t patch_song[NUM_PATCHES]

	for(int i=0;i<NUM_PATCHES;i++)
	{
		if(patch_song[i]<0 || patch_song[i]>=NUM_PATCHES)
		{
			#ifdef DEBUG_OUTPUT
			printf("load_current_patch(%d): fixing corrupt data in patch_song[%d]: %d => %d\n", position, i, patch_song[i], i);
			#endif
			patch_song[i] = i;
		}
	}

	bit1 = b_patch->bit1;
	bit2 = b_patch->bit2;
	memcpy(var_p, b_patch->var_p, 4); //unsigned char var_p[4];
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): bit & var = [%d,%d], [%d,%d,%d,%d]\n", position, bit1, bit2, var_p[0], var_p[1], var_p[2], var_p[3]);
	#endif

	arpeggiator = b_patch->arpeggiator;
	if(arpeggiator<ARP_NONE) { arpeggiator = ARP_NONE; }
	if(arpeggiator>ARP_MAX) { arpeggiator = ARP_MAX; }
	memcpy(arp_pattern, b_patch->arp_pattern, (ARP_MAX_STEPS+1)*sizeof(int)); //int arp_pattern[ARP_MAX_STEPS+1]

	//arp_active_steps = b_patch->arp_active_steps;
	reset_arp_active_steps();

	arp_octave = b_patch->arp_octave;
	arp_repeat = b_patch->arp_repeat;
	arp_range_from = b_patch->arp_range_from;
	arp_range_to = b_patch->arp_range_to;
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): arp = %d, a/o/rp/rg = [%d,%d,%d,(%d)<->(%d)]\n", position, arpeggiator, arp_active_steps, arp_octave, arp_repeat, arp_range_from, arp_range_to);
	#endif

	//constrain octave to min-max ranges
	//if(arp_octave<ARP_OCTAVE_RANGE_MIN) { arp_octave = ARP_OCTAVE_RANGE_MIN; }
	//if(arp_octave>ARP_OCTAVE_RANGE_MAX) { arp_octave = ARP_OCTAVE_RANGE_MAX; }
	//constrain octave min-max ranges to absolute limits
	if(arp_range_from>arp_range_to) { arp_range_from = arp_range_to; } //prevents divide by zero in random arpeggiator mode
	if(arp_range_from<ARP_OCTAVE_RANGE_MIN) { arp_range_from = ARP_OCTAVE_RANGE_MIN; }
	if(arp_range_from>ARP_OCTAVE_RANGE_MAX) { arp_range_from = ARP_OCTAVE_RANGE_MAX; }
	if(arp_range_to<ARP_OCTAVE_RANGE_MIN) { arp_range_to = ARP_OCTAVE_RANGE_MIN; }
	if(arp_range_to>ARP_OCTAVE_RANGE_MAX) { arp_range_to = ARP_OCTAVE_RANGE_MAX; }
	//constrain octave to user defined ranges
	if(arp_octave<arp_range_from) { arp_octave = arp_range_from; }
	if(arp_octave>arp_range_to) { arp_octave = arp_range_to; }
	//constrain arp repeat value
	if(arp_repeat<0) { arp_repeat = 0; }
	if(arp_repeat>ARP_REPEAT_MAX) { arp_repeat = ARP_REPEAT_MAX; }
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): arp[constrained] o/rp/rg = [%d,%d,(%d)<->(%d)]\n", position, arp_octave, arp_repeat, arp_range_from, arp_range_to);
	#endif
	arp_octave_effective = arp_octave;

	layer = b_patch->layer;

	layers_active = b_patch->layers_active;
	arp_layer = b_patch->arp_layer;
	if(arp_layer>7) { arp_layer = layer; }
	seq_layer = b_patch->seq_layer;
	if(seq_layer>7) { seq_layer = layer; }

	if(layers_active&0x80) //if flash layer active
	{
		flash_map_samples();
		//FLASH_SAMPLE_FORMAT = get_sample_format();
	}

	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): layer = %d, layers_active = %02x, arp_layer = %d, seq_layer = %d\n", position, layer, layers_active, arp_layer, seq_layer);
	#endif

	SEQUENCER_TIMING = b_patch->SEQUENCER_TIMING;
	if(SEQUENCER_TIMING<SEQUENCER_TIMING_MIN) { SEQUENCER_TIMING = SEQUENCER_TIMING_MIN; }
	bytebeat_echo_on = b_patch->bytebeat_echo_on;
	echo_dynamic_loop_current_step = b_patch->echo_dynamic_loop_current_step;
	echo_dynamic_loop_length = b_patch->echo_dynamic_loop_length;
	if(echo_dynamic_loop_length<=0) { bytebeat_echo_on = 0; echo_dynamic_loop_length = 0; echo_dynamic_loop_current_step = ECHO_DYNAMIC_LOOP_STEP_OFF; }
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): tempo = %d, echo = [on=%d,%d(%d)]\n", position, SEQUENCER_TIMING, bytebeat_echo_on, echo_dynamic_loop_current_step, echo_dynamic_loop_length);
	#endif

	sensors_active = b_patch->sensors_active;
	sensors_settings_shift = b_patch->sensors_settings_shift;
	sensors_settings_div = b_patch->sensors_settings_div;
	sensor_base = b_patch->sensor_base;
	sensor_range = b_patch->sensor_range;
	if(sensor_range<SENSOR_RANGE_MIN || sensor_range>SENSOR_RANGE_MAX)
	{
		sensor_range = SENSOR_RANGE_DEFAULT;
		sensor_base = SENSOR_BASE_DEFAULT;
	}
	sensors_wt_layers = b_patch->sensors_wt_layers;
	if(sensors_wt_layers>3)
	{
		sensors_wt_layers = 0; //in wave table layers, by default both sensors off
	}
	wt_cycle_alignment = b_patch->wt_cycle_alignment;
	if(wt_cycle_alignment>3)
	{
		wt_cycle_alignment = WT_CYCLE_ALIGNMENT_DEFAULT;
	}
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): sensors_active/sensors_settings_shift/sensors_settings_div = %d/%d/%d, base/range = %d/%f, sensors_wt_layers = %02x, wt_cycle_alignment = %02x\n", position, sensors_active, sensors_settings_shift, sensors_settings_div, sensor_base, sensor_range, sensors_wt_layers, wt_cycle_alignment);
	#endif

	bytebeat_song_pos_shift = b_patch->bytebeat_song_pos_shift;
	bytebeat_song_length_div = b_patch->bytebeat_song_length_div;
	bb_ptr_continuous = b_patch->bb_ptr_continuous;
	if(bytebeat_song_length_div==0)bytebeat_song_length_div = 1; //just in case the patch is corrrupt
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): bytebeat_song_pos_shift/bytebeat_song_length_div = %d/%d, bb_ptr_continuous = %d\n", position, bytebeat_song_pos_shift, bytebeat_song_length_div, bb_ptr_continuous);
	#endif

	sequencer = b_patch->sequencer;
	//printf("load_current_patch(%d): seq = %d\n", position, sequencer);
	memcpy(sequencer_steps, b_patch->sequencer_steps, (NUM_SEQUENCES)*sizeof(int8_t)); //int8_t sequencer_steps[NUM_SEQUENCES]
	memcpy(seq_steps_r1, b_patch->seq_steps_r1, (NUM_SEQUENCES)*sizeof(int8_t)); //int8_t seq_steps_r1[NUM_SEQUENCES]
	memcpy(seq_steps_r2, b_patch->seq_steps_r2, (NUM_SEQUENCES)*sizeof(int8_t)); //int8_t seq_steps_r2[NUM_SEQUENCES]
	//memcpy(seq_pattern, b_patch->seq_pattern, (NUM_SEQUENCES)*sizeof(int8_t*)); //int8_t* seq_pattern[NUM_SEQUENCES];
	memcpy(seq_filled_steps, b_patch->seq_filled_steps, (NUM_SEQUENCES)*sizeof(int8_t)); //int8_t seq_filled_steps[NUM_SEQUENCES]

	seq_octave = b_patch->seq_octave;
	seq_octave_mode = b_patch->seq_octave_mode;
	seq_range_from = b_patch->seq_range_from;
	seq_range_to = b_patch->seq_range_to;
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): seq = %d, o/m/rg = [%d,%d,(%d)<->(%d)]\n", position, sequencer, seq_octave, seq_octave_mode, seq_range_from, seq_range_to);
	#endif

	if(seq_octave<SEQ_OCTAVE_RANGE_MIN) { seq_octave = SEQ_OCTAVE_RANGE_MIN; }
	if(seq_octave>SEQ_OCTAVE_RANGE_MAX) { seq_octave = SEQ_OCTAVE_RANGE_MAX; }
	if(seq_range_from>seq_range_to) { seq_range_from = seq_range_to; } //prevents divide by zero
	if(seq_range_from<SEQ_OCTAVE_RANGE_MIN) { seq_range_from = SEQ_OCTAVE_RANGE_MIN; }
	if(seq_range_from>SEQ_OCTAVE_RANGE_MAX) { seq_range_from = SEQ_OCTAVE_RANGE_MAX; }
	if(seq_range_to<SEQ_OCTAVE_RANGE_MIN) { seq_range_to = SEQ_OCTAVE_RANGE_MIN; }
	if(seq_range_to>SEQ_OCTAVE_RANGE_MAX) { seq_range_to = SEQ_OCTAVE_RANGE_MAX; }
	if(seq_octave_mode<0) { seq_octave_mode = 0; }
	if(seq_octave_mode>SEQ_OCTAVE_MODES_MAX) { seq_octave_mode = SEQ_OCTAVE_MODES_MAX; }
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): seq[constrained] o/m/rg = [%d,%d,(%d)<->(%d)]\n", position, seq_octave, seq_octave_mode, seq_range_from, seq_range_to);
	#endif
	seq_octave_effective = seq_octave;

	memcpy(mini_piano_tuning, b_patch->mini_piano_tuning, (MINI_PIANO_KEYS)*sizeof(uint16_t)); //uint16_t mini_piano_tuning[MINI_PIANO_KEYS]
	memcpy(rnd_envelope_div, b_patch->rnd_envelope_div, (MINI_PIANO_KEYS)*sizeof(int8_t)); //int8_t rnd_envelope_div[MINI_PIANO_KEYS]

	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): unpacking mini_piano_waves variable: ", position);
	#endif
	for(int i=0;i<MINI_PIANO_KEYS/2;i++)
	{
		//uint8_t mini_piano_waves[MINI_PIANO_KEYS]
		mini_piano_waves[2*i] = b_patch->mini_piano_waves[i] % 16;
		mini_piano_waves[2*i+1] = b_patch->mini_piano_waves[i] / 16;
		#ifdef DEBUG_OUTPUT
		printf("%02x -> (%x-%x)	", b_patch->mini_piano_waves[i], mini_piano_waves[2*i], mini_piano_waves[2*i+1]);
		#endif
	}
	#ifdef DEBUG_OUTPUT
	printf("\n");
	#endif

	WAVESAMPLE_BOOST_VOLUME = b_patch->WAVESAMPLE_BOOST_VOLUME;
	FLASH_SAMPLE_BOOST_VOLUME = b_patch->FLASH_SAMPLE_BOOST_VOLUME;
	if(WAVESAMPLE_BOOST_VOLUME < WAVESAMPLE_BOOST_VOLUME_MIN || WAVESAMPLE_BOOST_VOLUME > WAVESAMPLE_BOOST_VOLUME_MAX)
	{
		WAVESAMPLE_BOOST_VOLUME = WAVESAMPLE_BOOST_VOLUME_DEFAULT;
	}
	if(FLASH_SAMPLE_BOOST_VOLUME < FLASH_SAMPLE_BOOST_VOLUME_MIN || FLASH_SAMPLE_BOOST_VOLUME > FLASH_SAMPLE_BOOST_VOLUME_MAX)
	{
		FLASH_SAMPLE_BOOST_VOLUME = FLASH_SAMPLE_BOOST_VOLUME_DEFAULT;
	}
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): WAVESAMPLE_BOOST_VOLUME = %d, FLASH_SAMPLE_BOOST_VOLUME = %d\n", position, WAVESAMPLE_BOOST_VOLUME, FLASH_SAMPLE_BOOST_VOLUME);
	#endif

	free(b_patch);

	sample_patch_t *s_patch;
	s_patch = (sample_patch_t*)malloc(sizeof(sample_patch_t));
	int bytes_loaded = load_patch_nvs((void*)s_patch, position+bank*8, "smp", sizeof(sample_patch_t));
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): load_patch_nvs(s_patch,...) bytes_loaded = %d\n", position, bytes_loaded);
	#endif

	layer7_samples_delete = 0;
	if(bytes_loaded == sizeof(sample_patch_t))
	{
		layer7_patch_modified = 1;
		memcpy(layer7_patch_start, s_patch->layer7_patch_start, NUM_PATCHES*sizeof(int)); //int layer7_patch_start[NUM_PATCHES];
		memcpy(layer7_patch_length, s_patch->layer7_patch_length, NUM_PATCHES*sizeof(int)); //int layer7_patch_length[NUM_PATCHES];
		memcpy(layer7_patch_bitrate, s_patch->layer7_patch_bitrate, NUM_PATCHES*sizeof(int)); //int layer7_patch_bitrate[NUM_PATCHES];
		#ifdef DEBUG_OUTPUT
		printf("load_current_patch(%d): load_patch_nvs(s_patch,...) returned correct amount of bytes\n", position);
		#endif
	}
	else
	{
		layer7_patch_modified = 0;
		#ifdef DEBUG_OUTPUT
		printf("load_current_patch(%d): load_patch_nvs(s_patch,...) returned unexpected or zero amount of bytes\n", position);
		#endif
	}
	free(s_patch);

	#ifdef DEBUG_OUTPUT
	//display sequencer stats
	printf("load_current_patch(%d): sequencer_steps={", position);
	for(int i=0;i<NUM_SEQUENCES;i++){printf("%d,",sequencer_steps[i]);}
	printf("}\nload_current_patch(%d): seq_steps_r1={", position);
	for(int i=0;i<NUM_SEQUENCES;i++){printf("%d,",seq_steps_r1[i]);}
	printf("}\nload_current_patch(%d): seq_steps_r2={", position);
	for(int i=0;i<NUM_SEQUENCES;i++){printf("%d,",seq_steps_r2[i]);}
	printf("}\nload_current_patch(%d): seq_filled_steps={", position);
	for(int i=0;i<NUM_SEQUENCES;i++){printf("%d,",seq_filled_steps[i]);}
	printf("}\n");
	#endif

	//load sequences
	load_sequences_nvs(position+bank*8);

	//reset some variables to defaults
	arp_seq_dir = 1;
	arp_repeat = 0;
	arp_ptr = -1;
	arp_rep_cnt = 0;
	//layer = 0;
	patch = -1; //no recent patch, to prevent assigning modified params in store function

	//sequencer = 0;
	if(sequencer<0 || sequencer > NUM_SEQUENCES) { sequencer = 0; }
	if(sequencer>0 && seq_filled_steps[sequencer-1]<=0) { sequencer = 0; }
	start_stop_sequencer(sequencer);
	//sequencer_blink = 0; //possibly not used
	//sequencer_counter = 0;
	#ifdef DEBUG_OUTPUT
	printf("load_current_patch(%d): seq[final] = %d, running = %d\n", position, sequencer, seq_running);
	printf("load_current_patch(%d): seq_filled_steps={", position);
	for(int i=0;i<NUM_SEQUENCES;i++){printf("%d,",seq_filled_steps[i]);}
	printf("}\n");
	#endif

	encoder_blink = 0;

	sampleArpSeq = 0;
	sampleCounter = 0;
}

uint8_t find_saved_patches(int bank)
{
	esp_err_t res;

	/*
	//----------------------------------------
	nvs_stats_t nvs_stats;
	res = nvs_get_stats(NULL, &nvs_stats);
	printf("find_saved_patches(%d): nvs_stats = {C=%d,F=%d,T=%d,U=%d}\n", bank, nvs_stats.namespace_count, nvs_stats.free_entries, nvs_stats.total_entries, nvs_stats.used_entries);
	if(res!=ESP_OK)
	{
		printf("find_saved_patches(%d): problem with nvs_get_stats(), error = %d\n", bank, res);
		return 0;
	}

	nvs_iterator_t it;
	nvs_entry_info_t out_info;
	it = nvs_entry_find("nvs", "bb_settings", NVS_TYPE_BLOB);
	if(it)
	{
		nvs_entry_info(it, &out_info);
		printf("find_saved_patches(%d): nvs_entry_find returned = {K=%s,N=%s,T=%x}\n", bank, out_info.key, out_info.namespace_name, out_info.type);
		while(it)
		{
			it = nvs_entry_next(it);
			if(it)
			{
				nvs_entry_info(it, &out_info);
				printf("find_saved_patches(%d): nvs_entry_next returned = {K=%s,N=%s,T=%x}\n", bank, out_info.key, out_info.namespace_name, out_info.type);
			}
		}
	}
	//----------------------------------------
	*/

	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("find_saved_patches(%d): problem with nvs_open(), error = %d\n", bank, res);
		return 0;
	}

	size_t used_entries;
	res = nvs_get_used_entry_count(handle, &used_entries);
	//printf("find_saved_patches(%d): used_entries = %d\n", bank, used_entries);
	if(res!=ESP_OK)
	{
		printf("find_saved_patches(%d): problem with nvs_get_used_entry_count(), error = %d\n", bank, res);
		return 0;
	}

	bytebeat_patch_t *b_patch;
	b_patch = (bytebeat_patch_t*)malloc(sizeof(bytebeat_patch_t));

	uint8_t result = 0;

	char key[20];
	size_t bytes_loaded = -1;
	for(int i=0;i<PATCHES_PER_BANK;i++)
	{
		sprintf(key,"patch%d",i+1+bank*8);
		bytes_loaded = -1;
		res = nvs_get_blob(handle, key, b_patch, &bytes_loaded);
		if(res!=ESP_OK) //problem reading data
		{
			#ifdef DEBUG_OUTPUT
			printf("patch \"%s\" not found\n", key);
			//printf("find_saved_patches(%d): problem reading key \"%s\", bank, error = %d\n", key, res);
			#endif
		}
		else if(bytes_loaded==sizeof(bytebeat_patch_t))
		{
			#ifdef DEBUG_OUTPUT
			printf("patch \"%s\" found\n", key);
			#endif
			result |= (1<<i);
		}
		else
		{
			#ifdef DEBUG_OUTPUT
			printf("patch \"%s\" found but has wrong length\n", key);
			#endif
			result |= (1<<i); //if we need backward compatibiliy temporarily after patch structure changed
		}
	}
	nvs_close(handle);
	free(b_patch);

	return result;
}

#ifdef PERSISTENT_SETTINGS
void store_persistent_settings(persistent_settings_t *settings)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("store_persistent_settings(): problem with nvs_open(), error = %d\n", res);
		return;
	}
	//nvs_erase_all(handle); //testing

	if(settings->VOLUME_updated)
	{
		printf("store_persistent_settings(): VOLUME(DV) updated to %d\n", settings->VOLUME);
		settings->VOLUME_updated = 0;
		res = nvs_set_i16(handle, "DV", settings->VOLUME);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i16(), error = %d\n", res);
		}
	}
	if(settings->TEMPO_updated)
	{
		printf("store_persistent_settings(): TEMPO updated to %d\n", settings->TEMPO);
		settings->TEMPO_updated = 0;
		res = nvs_set_i32(handle, "TEMPO", settings->TEMPO);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i16(), error = %d\n", res);
		}
	}
	/*
	if(settings->FINE_TUNING_updated)
	{
		printf("store_persistent_settings(): FINE_TUNING(TUNING) updated to %f\n", settings->FINE_TUNING);
		settings->FINE_TUNING_updated = 0;
		uint64_t *val_u64 = (uint64_t*)&settings->FINE_TUNING;
		res = nvs_set_u64(handle, "TUNING", val_u64[0]);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i16(), error = %d\n", res);
		}
	}
	if(settings->TRANSPOSE_updated)
	{
		printf("store_persistent_settings(): TRANSPOSE updated to %d\n", settings->TRANSPOSE);
		settings->TRANSPOSE_updated = 0;
		res = nvs_set_i8(handle, "TRN", settings->TRANSPOSE);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}
	*/
	if(settings->SENSORS_updated)
	{
		printf("store_persistent_settings(): SENSORS updated to %d\n", settings->SENSORS);
		settings->SENSORS_updated = 0;
		res = nvs_set_i8(handle, "SENSORS", settings->SENSORS);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}

	res = nvs_commit(handle);
	if(res!=ESP_OK) //problem writing data
	{
		printf("store_persistent_settings(): problem with nvs_commit(), error = %d\n", res);
	}
	nvs_close(handle);
}

void load_persistent_settings(persistent_settings_t *settings)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("load_persistent_settings(): problem with nvs_open() in ");
		printf("R/O mode, error = %d\n", res);

		//maybe it was not created yet, will try
		res = nvs_open("bb_settings", NVS_READWRITE, &handle);
		if(res!=ESP_OK)
		{
			printf("load_persistent_settings(): problem with nvs_open() in ");
			printf("R/W mode, error = %d\n", res);
			return;
		}
	}

	int8_t val_i8;
	int16_t val_i16;
	int32_t val_i32;
	//uint64_t val_u64;

	res = nvs_get_i16(handle, "DV", &val_i16);
	if(res!=ESP_OK) { val_i16 = BB_VOLUME_DEFAULT; } //if the key does not exist, load default value
	settings->VOLUME = val_i16;

	res = nvs_get_i32(handle, "TEMPO", &val_i32);
	if(res!=ESP_OK) {
		val_i32 = SEQUENCER_TIMING_DEFAULT;
		printf("load_persistent_settings(): TEMPO not found, loading default = %d\n", val_i32);
	}
	settings->TEMPO = val_i32;

	/*
	res = nvs_get_u64(handle, "TUNING", &val_u64);
	if(res!=ESP_OK) {
		settings->FINE_TUNING = global_settings.TUNING_DEFAULT;
		printf("load_persistent_settings(): FINE_TUNING(TUNING) not found, loading default = %f\n", global_settings.TUNING_DEFAULT);
	}
	else
	{
		settings->FINE_TUNING = ((double*)&val_u64)[0];
	}

	res = nvs_get_i8(handle, "TRN", &val_i8);
	if(res!=ESP_OK) {
		val_i8 = 0;
		printf("load_persistent_settings(): TRANSPOSE not found, loading default = %d\n", val_i8);
	}
	settings->TRANSPOSE = val_i8;
	*/

	res = nvs_get_i8(handle, "SENSORS", &val_i8);
	if(res!=ESP_OK) { val_i8 = SENSORS_ACTIVE_BOTH; }
	settings->SENSORS = val_i8;

	nvs_close(handle);
}

int load_all_settings()
{
    load_persistent_settings(&persistent_settings);
    printf("Persistent settings loaded: VOLUME=%d, TEMPO=%d, "//TRANSPOSE=%d, FINE_TUNING=%f,"
    		"SENSORS=%d\n",
    		persistent_settings.VOLUME,
			persistent_settings.TEMPO,
			//persistent_settings.TRANSPOSE,
			//persistent_settings.FINE_TUNING,
			persistent_settings.SENSORS);

    BB_SHIFT_VOLUME = persistent_settings.VOLUME;
    SEQUENCER_TIMING = persistent_settings.TEMPO;
    sensors_active = persistent_settings.SENSORS;
    //transpose = persistent_settings.TRANSPOSE;
    //tuning = persistent_settings.FINE_TUNING;

    return 0;
}

/*
void persistent_settings_store_eq()
{
	persistent_settings.EQ_BASS = EQ_bass_setting;
	persistent_settings.EQ_BASS_updated = 1;
	persistent_settings.EQ_TREBLE = EQ_treble_setting;
	persistent_settings.EQ_TREBLE_updated = 1;
	persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
}
*/
#endif

int nvs_erase_namespace(char *namespace)
{
	esp_err_t res;
	nvs_handle handle;

	res = nvs_open(namespace, NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("nvs_erase_namespace(): problem with nvs_open(), error = %d\n", res);
		//indicate_error(0x0001, 10, 100);
		return 1;
	}
	res = nvs_erase_all(handle);
	if(res!=ESP_OK)
	{
		printf("nvs_erase_namespace(): problem with nvs_erase_all(), error = %d\n", res);
		//indicate_error(0x0003, 10, 100);
		return 2;
	}
	res = nvs_commit(handle);
	if(res!=ESP_OK)
	{
		printf("nvs_erase_namespace(): problem with nvs_commit(), error = %d\n", res);
		//indicate_error(0x0007, 10, 100);
		return 3;
	}
	nvs_close(handle);

	return 0;
}

void settings_reset()
{
	//LEDs_all_OFF();

	if(!nvs_erase_namespace("bb_settings")) //if no error
	{
		//indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_LEFT_8, 4, 50);
		MCU_restart();
	}
}

void settings_stats()
{
	esp_err_t res;

	//----------------------------------------
	nvs_stats_t nvs_stats;
	res = nvs_get_stats(NULL, &nvs_stats);
	printf("settings_stats(): nvs_stats = {Count=%d,Free=%d,Total=%d,Used=%d}\n", nvs_stats.namespace_count, nvs_stats.free_entries, nvs_stats.total_entries, nvs_stats.used_entries);
	if(res!=ESP_OK)
	{
		printf("settings_stats(): problem with nvs_get_stats(), error = %d\n", res);
		return;
	}

	nvs_iterator_t it;
	nvs_entry_info_t *out_info = (nvs_entry_info_t*)malloc(sizeof(nvs_entry_info_t));

	it = nvs_entry_find("nvs", "bb_settings", NVS_TYPE_BLOB);
	if(!it)
	{
		printf("settings_stats(): nvs_entry_find returned NULL\n");
	}
	else
	{
		nvs_entry_info(it, out_info);
		printf("settings_stats(): nvs_entry_find returned = {K=%s,N=%s,T=%x}\n", out_info->key, out_info->namespace_name, out_info->type);
		while(it)
		{
			it = nvs_entry_next(it);
			if(it)
			{
				nvs_entry_info(it, out_info);
				printf("settings_stats(): nvs_entry_next returned = {K=%s,N=%s,T=%x}\n", out_info->key, out_info->namespace_name, out_info->type);
			}
		}
	}
	free(out_info);
	//----------------------------------------

	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("settings_stats(): problem with nvs_open(), error = %d\n", res);
		return;
	}

	size_t used_entries;
	res = nvs_get_used_entry_count(handle, &used_entries);
	printf("settings_stats(): used_entries = %d\n", used_entries);
	if(res!=ESP_OK)
	{
		printf("settings_stats(): problem with nvs_get_used_entry_count(), error = %d\n", res);
		return;
	}
	/*
	bytebeat_patch_t *b_patch;
	b_patch = (bytebeat_patch_t*)malloc(sizeof(bytebeat_patch_t));

	uint8_t result = 0;

	char key[20];
	size_t bytes_loaded = -1;
	for(int i=0;i<8;i++)
	{
		sprintf(key,"patch%d",i+1);
		bytes_loaded = -1;
		res = nvs_get_blob(handle, key, b_patch, &bytes_loaded);
		if(res!=ESP_OK) //problem reading data
		{
			printf("patch \"%s\" not found\n", key);
			//printf("find_saved_patches(): problem reading key \"%s\", error = %d\n", key, res);
		}
		else if(bytes_loaded==sizeof(bytebeat_patch_t))
		{
			printf("patch \"%s\" found\n", key);
			result |= (1<<i);
		}
		else
		{
			printf("patch \"%s\" found but has wrong length\n", key);
			result |= (1<<i); //if we need backward compatibiliy temporarily after patch structure changed
		}
	}
	free(b_patch);
	*/

	nvs_close(handle);
}

void delete_patch_nvs(int position, int delete_patch, int delete_samples, int delete_sequences)
{
	if(delete_patch && (!delete_samples || !delete_sequences))
	{
		printf("delete_patch_nvs(): cannot leave orphans! delete_patch = %d but delete_samples = %d, delete_sequences = %d\n", delete_patch, delete_samples, delete_sequences);
		delete_samples = 1;
		delete_sequences = 1;
	}

	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("delete_patch_nvs(): problem with nvs_open(), error = %d\n", res);
		return;
	}

	char key[20];

	if(delete_patch)
	{
		sprintf(key,"patch%d",position);

		#ifdef DEBUG_OUTPUT
		printf("delete_patch_nvs(): deleting key \"%s\"\n", key);
		#endif

		res = nvs_erase_key(handle, key);
		if(res!=ESP_OK) //problem deleting data
		{
			printf("delete_patch_nvs(): problem with nvs_erase_key() while deleting key \"%s\", error = %d\n", key, res);
			nvs_close(handle);
			return;
		}
	}

	if(delete_samples)
	{
		//also delete sample patch if present
		sprintf(key,"smp%d",position);

		#ifdef DEBUG_OUTPUT
		printf("delete_patch_nvs(): deleting key \"%s\"\n", key);
		#endif

		res = nvs_erase_key(handle, key);
		if(res!=ESP_OK) //problem deleting data
		{
			printf("delete_patch_nvs(): problem with nvs_erase_key() while deleting key \"%s\", error = %d\n", key, res);
			//nvs_close(handle);
			//return;
		}
	}

	if(delete_sequences)
	{
		//erase all sequences too
		for(int seq=0;seq<NUM_SEQUENCES;seq++)
		{
			sprintf(key,"seq%d_%d",position,seq);
			#ifdef DEBUG_OUTPUT
			printf("delete_patch_nvs(): deleting key \"%s\"\n", key);
			#endif
			res = nvs_erase_key(handle, key);
			if(res!=ESP_OK) //problem deleting data
			{
				printf("delete_patch_nvs(): problem with nvs_erase_key() while deleting key \"%s\", error = %d\n", key, res);
				//nvs_close(handle);
				//return;
			}
		}
	}

	res = nvs_commit(handle);
	if(res!=ESP_OK) //problem deleting data
	{
		printf("delete_patch_nvs(): problem with nvs_commit() while deleting key \"%s\", error = %d\n", key, res);
	}

	nvs_close(handle);
}

void delete_last_loaded_patch()
{
	if(last_loaded_patch>=0)
	{
		#ifdef DEBUG_OUTPUT
		printf("delete_patch_nvs(): deleting patch %d\n", last_loaded_patch);
		#endif
		delete_patch_nvs(last_loaded_patch, 1, 1, 1);
	}
	else
	{
		#ifdef DEBUG_OUTPUT
		printf("delete_patch_nvs(): there was no patch loaded recently\n");
		#endif
	}
}

void delete_sequence_nvs(int position, int seq_id)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("delete_sequence_nvs(): problem with nvs_open(), error = %d\n", res);
		return;
	}

	char key[20];
	sprintf(key,"seq%d_%d",position,seq_id);
	#ifdef DEBUG_OUTPUT
	printf("delete_sequence_nvs(): deleting key \"%s\"\n", key);
	#endif

	res = nvs_erase_key(handle, key);
	if(res!=ESP_OK) //problem deleting data
	{
		printf("delete_sequence_nvs(): problem with nvs_erase_key() while deleting key \"%s\", error = %d\n", key, res);
		nvs_close(handle);
		return;
	}

	res = nvs_commit(handle);
	if(res!=ESP_OK) //problem deleting data
	{
		printf("delete_sequence_nvs(): problem with nvs_commit() while deleting key \"%s\", error = %d\n", key, res);
	}

	nvs_close(handle);
}

void delete_sequence(int sequence_id)
{
	if(last_loaded_patch>=0)
	{
		#ifdef DEBUG_OUTPUT
		printf("delete_patch_nvs(%d): deleting sequence %d in patch %d\n", sequence_id, sequence_id-1, last_loaded_patch);
		#endif
		delete_sequence_nvs(last_loaded_patch, sequence_id-1);
	}
	else
	{
		#ifdef DEBUG_OUTPUT
		printf("delete_sequence(%d): there was no patch loaded recently\n", sequence_id);
		#endif
	}
}

#define PATCH_PARTITION_COPY_BUFFER 0x10000

void backup_restore_patches(const char *src, const char *dst)
{
	printf("backup_restore_patches(%s -> %s)\n", src, dst);

	nvs_flash_deinit();

	const esp_partition_t *pt_src, *pt_dst;
	pt_src = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, src);
	pt_dst = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, dst);

	if(pt_src==NULL)
	{
		printf("backup_restore_patches(): esp_partition_find_first[src] returned NULL\n");
		return;
	}
	printf("backup_restore_patches(): esp_partition_find_first[src] found a partition with size = 0x%x, label = %s, addres=0x%x\n", pt_src->size, pt_src->label, pt_src->address);

	if(pt_dst==NULL)
	{
		printf("backup_restore_patches(): esp_partition_find_first[dst] returned NULL\n");
		return;
	}
	printf("backup_restore_patches(): esp_partition_find_first[dst] found a partition with size = 0x%x, label = %s, addres=0x%x\n", pt_dst->size, pt_dst->label, pt_dst->address);

	int free_mem = xPortGetFreeHeapSize();
	printf("backup_restore_patches(): free mem = %d\n", free_mem);
	char *buf = malloc(PATCH_PARTITION_COPY_BUFFER);

	if(buf==NULL)
	{
		printf("backup_restore_patches(): malloc(%d) returned NULL\n", PATCH_PARTITION_COPY_BUFFER);
		return;
	}

	free_mem = xPortGetFreeHeapSize();
	printf("backup_restore_patches(): free mem after allocating buffer = %d\n", free_mem);

	printf("backup_restore_patches(): nvs_flash_erase_partition: erasing dest partition \"%s\"\n", dst);
	esp_err_t res = nvs_flash_erase_partition(dst);
	if(res!=ESP_OK)
	{
		printf("backup_restore_patches(): nvs_flash_erase_partition returned code = %d\n", res);
		free(buf);
		return;
	}

	int steps = pt_src->size / PATCH_PARTITION_COPY_BUFFER;
	int leftover = pt_src->size % PATCH_PARTITION_COPY_BUFFER;
	printf("backup_restore_patches(): steps=%d, leftover=%d\n", steps, leftover);

	for(int i=0;i<=steps;i++)
	{
		printf("backup_restore_patches(): step=%d, address=%x, size=%d\n", i, i*PATCH_PARTITION_COPY_BUFFER, i==steps?leftover:PATCH_PARTITION_COPY_BUFFER);
		res = esp_partition_read(pt_src, i*PATCH_PARTITION_COPY_BUFFER, buf, i==steps?leftover:PATCH_PARTITION_COPY_BUFFER);
		if(res==ESP_ERR_INVALID_SIZE)
		{
			printf("backup_restore_patches(): esp_partition_read returned code ESP_ERR_INVALID_SIZE\n");
			free(buf);
			return;
		}
		else if(res!=ESP_OK)
		{
			printf("backup_restore_patches(): esp_partition_read returned code = %d\n", res);
			free(buf);
			return;
		}

		res = esp_partition_write(pt_dst, i*PATCH_PARTITION_COPY_BUFFER, buf, i==steps?leftover:PATCH_PARTITION_COPY_BUFFER);
		if(res==ESP_ERR_INVALID_SIZE)
		if(res!=ESP_OK)
		{
			printf("backup_restore_patches(): esp_partition_write returned code = %d\n", res);
		}
	}

	free(buf);

	res = nvs_flash_init();
	if(res!=ESP_OK)
	{
		printf("backup_restore_patches(): nvs_flash_init returned code = %d\n", res);
	}
}

int self_tested()
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("self_test(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	#ifdef DEBUG_OUTPUT
	printf("self_test(): checking for \"unit_tested\" key\n");
	#endif

	int8_t tested;
	res = nvs_get_i8(handle, "unit_tested", &tested);

	if(res!=ESP_OK) //problem reading data
	{
		printf("self_test(): problem with nvs_get_i8() while reading key \"unit_tested\", error = %d\n", res);
		nvs_close(handle);
		return 0;
	}

	nvs_close(handle);
	return tested;
}

void nvs_set_flag(const char *key, int8_t value)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("nvs_set_flag(): problem with nvs_open(), error = %d\n", res);
		return;
	}

	#ifdef DEBUG_OUTPUT
	printf("nvs_set_flag(): creating key \"%s\" with value %d\n", key, value);
	#endif

	res = nvs_set_i8(handle, key, value);
	if(res!=ESP_OK) //problem writing data
	{
		printf("nvs_set_flag(): problem with nvs_set_i8() while creating key \"%s\", error = %d\n", key, res);
		nvs_close(handle);
		return;
	}

	res = nvs_commit(handle);
	if(res!=ESP_OK) //problem writing data
	{
		printf("nvs_set_flag(): problem with nvs_commit() while creating key \"%s\", error = %d\n", key, res);
	}

	nvs_close(handle);
}

int nvs_get_flag(const char *key) //returns 0 not only when value is zero but also when key not found
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("bb_settings", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("nvs_get_flag(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	#ifdef DEBUG_OUTPUT
	printf("nvs_get_flag(): checking for \"%s\" key\n", key);
	#endif

	int8_t val;
	res = nvs_get_i8(handle, key, &val);

	if(res!=ESP_OK) //problem reading data
	{
		printf("nvs_get_flag(): problem with nvs_get_i8() while reading key \"%s\", error = %d\n", key, res);
		nvs_close(handle);
		return 0;
	}

	nvs_close(handle);
	return val;
}

void store_selftest_pass(int value)
{
	nvs_set_flag("unit_tested", value);
}

void reset_selftest_pass()
{
	printf("reset_selftest_pass(): store_selftest_pass(0);\n");
	store_selftest_pass(0);
}

void set_sample_format(int format)
{
	nvs_set_flag("s_f", format);
}

int get_sample_format()
{
	return nvs_get_flag("s_f");
}

void set_encoder_steps_per_event(int steps)
{
	nvs_set_flag("e_st", steps);
}

int get_encoder_steps_per_event()
{
	return nvs_get_flag("e_st");
}

void set_bb_volume(int vol)
{
	nvs_set_flag("b_v", vol + 0x10); //add the constant to distinguish from '0' as value not set
}

int get_bb_volume()
{
	int vol = nvs_get_flag("b_v");
	if(vol)
	{
		return vol - 0x10;
	}
	//else if nothing stored yet
	return BB_VOLUME_DEFAULT;
}

