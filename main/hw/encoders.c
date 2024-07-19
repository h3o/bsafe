/*
 * encoders.c
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 28 Jan 2021
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

#include "encoders.h"

#include <math.h>

#include <init.h>
#include <signals.h>
#include <gpio.h>
#include <dsp/Bytebeat.h>
#include <dsp/SineWaves.h>
#include <hw/settings.h>
#include <hw/keys.h>

//#define DEBUG_OUTPUT

#define ENC_SPEED_SLOW 		10000
#define ENC_SPEED_NORMAL	2000
#define ENC_SPEED_FAST		1000

int32_t samples_since_last_enc_event[2] = {ENC_SPEED_NORMAL,ENC_SPEED_NORMAL};
int16_t encoder_speed[2] = {0,0};
float encoder_speed_multiplier = 1.0f;

void animate_encoder(int encoder, int leds)
{
	//printf("animate_encoder(%d, %x)\n", encoder, leds);
	led_disp[leds] &= 0x0f; //remove blink mask

	led_disp[leds]+=encoder_results[encoder];
	if(led_disp[leds]>4)
	{
		led_disp[leds]=1;
	}
	if(led_disp[leds]<1)
	{
		led_disp[leds]=4;
	}

	if(encoder_blink)
	{
		led_disp[leds] |= LEDS_BLINK; //add blink mask
	}
}

void change_bank(int change)
{
	bank += change;
	if(bank>=PATCH_BANKS)
	{
		bank = 0;
	}
	if(bank<0)
	{
		bank = PATCH_BANKS-1;
	}
	saved_patches=find_saved_patches(bank);
	#ifdef DEBUG_OUTPUT
	printf("change_bank(%d): find_saved_patches(): saved_patches = %x\n", change, saved_patches);
	#endif
	LEDS_display_direct(0x01<<bank,saved_patches); //orange showing bank
	#ifdef DEBUG_OUTPUT
	printf("change_bank(%d): find_saved_patches(): last_loaded_patch bank/position = %d/%d\n", change, (last_loaded_patch-1)/8, (last_loaded_patch-1)%8+1);
	#endif
	if(last_loaded_patch>=0 && (last_loaded_patch-1)/8==bank)
	{
		LEDS_display_direct_blink(0, (last_loaded_patch-1)%8+1); //indicate most recently loaded patch
	}
	else
	{
		LEDS_display_direct_blink(0, 0);
	}
}

void process_encoders()
{
	samples_since_last_enc_event[ENCODER_LEFT]++;
	samples_since_last_enc_event[ENCODER_RIGHT]++;

	if(encoder_results[ENCODER_LEFT]!=0)
	{
		keys_or_encoders_recently = ENCODERS_RECENTLY;
		last_ui_event = UI_EVENT_ENCODER;

		if(!menu_function && !shift_pressed && !set_pressed && !seq_running && !arpeggiator)
		{
			animate_encoder(ENCODER_LEFT,LEDS_BLUE1);
		}

		if(samples_since_last_enc_event[ENCODER_LEFT] > ENC_SPEED_SLOW)
		{
			encoder_speed[ENCODER_LEFT] = ENC_SPEED_SLOW;
		}
		else if(samples_since_last_enc_event[ENCODER_LEFT] > ENC_SPEED_NORMAL)
		{
			encoder_speed[ENCODER_LEFT] = ENC_SPEED_NORMAL;
		}
		else if(samples_since_last_enc_event[ENCODER_LEFT] < ENC_SPEED_FAST)
		{
			encoder_speed[ENCODER_LEFT] = ENC_SPEED_FAST;
		}

		//printf("samples_since_last_enc_event[ENCODER_LEFT]:%d, speed=%s\n", samples_since_last_enc_event[ENCODER_LEFT],
		//		(encoder_speed[ENCODER_LEFT]==ENC_SPEED_NORMAL?"NORMAL":"FAST"));

		samples_since_last_enc_event[ENCODER_LEFT] = 0;

		//printf("~L:%d/%d\n",leds_glowing[0],leds_glowing[1]);

		if((menu_function!=MENU_PLAY || seq_running || arpeggiator) && enc_function!=ENC_FUNCTION_ADJUST_TEMPO)
		{
			led_indication_refresh = 0; //in case menu is active
		}
		//----------------------------------------------------------------------------------------------------------------

		if(menu_function==MENU_SEQ && sequencer>0) //adjust sequencer steps count
		{
			seq_steps_r1[sequencer-1] += encoder_results[ENCODER_LEFT];
			if(seq_steps_r1[sequencer-1]<1) { seq_steps_r1[sequencer-1] = 1; }
			if(seq_steps_r1[sequencer-1]>8) { seq_steps_r1[sequencer-1] = 8; }

			sequencer_steps[sequencer-1]=seq_steps_r1[sequencer-1]*seq_steps_r2[sequencer-1];
			#ifdef DEBUG_OUTPUT
			printf("process_encoders(): sequencer_steps[%d] => %d (%dx%d)\n", sequencer-1, sequencer_steps[sequencer-1], seq_steps_r1[sequencer-1], seq_steps_r2[sequencer-1]);
			#endif
			indicate_sequencer_steps(seq_steps_r1[sequencer-1], seq_steps_r2[sequencer-1]);

			if(seq_layer==0)
			{
				trigger_patch(PATCH_SILENCE);
			}
			seq_running = 0;

			//led_indication_refresh = 0; //in case menu is active
		}

		if(menu_function==KEY_SET_HELD || menu_function==KEY_SHIFT_HELD)
		{
			change_bank(encoder_results[ENCODER_LEFT]);
			#ifdef DEBUG_OUTPUT
			printf("process_encoders(): bank -> %d\n", bank);
			printf("process_encoders(): find_saved_patches(): saved_patches = %x\n", saved_patches);
			#endif
		}

		if(menu_function==MENU_PLAY && shift_pressed && ((sequencer && seq_running) || arpeggiator)) //adjust tempo
		{
			if(encoder_speed[ENCODER_LEFT]==ENC_SPEED_SLOW)
			{
				SEQUENCER_TIMING -= encoder_results[ENCODER_LEFT] * 16;
			}
			else if(encoder_speed[ENCODER_LEFT]==ENC_SPEED_FAST)
			{
				SEQUENCER_TIMING -= encoder_results[ENCODER_LEFT] * 1024;
			}
			else
			{
				SEQUENCER_TIMING -= encoder_results[ENCODER_LEFT] * 128;
			}
			if(SEQUENCER_TIMING<SEQUENCER_TIMING_MIN)
			{
				SEQUENCER_TIMING = SEQUENCER_TIMING_MIN;
			}

			LEDS_BLUE_OFF;
			led_indication_refresh = -2; //block LED refresh until shift released
			enc_function = ENC_FUNCTION_ADJUST_TEMPO;
			enc_tempo_mult = 0;
			#ifdef DEBUG_OUTPUT
			printf("process_encoders(): encoder_speed[ENCODER_LEFT] = %d, SEQUENCER_TIMING => %d\n", encoder_speed[ENCODER_LEFT], SEQUENCER_TIMING);
			#endif
		}

		//do not allow any further processing of encoder events
		if(enc_function==ENC_FUNCTION_ADJUST_TEMPO)
		{
			encoder_results[ENCODER_LEFT] = 0;
			encoder_results[ENCODER_RIGHT] = 0;
			return; //to not process anything further in this function
		}

		/*
		if(set_pressed && sequencer>0) //adjust sequencer steps count
		{
			seq_steps_r1 += encoder_results[ENCODER_LEFT];
			if(seq_steps_r1<1) { seq_steps_r1 = 1; }
			if(seq_steps_r1>8) { seq_steps_r1 = 8; }
			sequencer_steps[sequencer-1]=seq_steps_r1*seq_steps_r2;
			printf("process_encoders(): sequencer_steps[%d] => %d\n", sequencer-1, sequencer_steps[sequencer-1]);
			indicate_sequencer_steps(seq_steps_r1, seq_steps_r2);
		}
		*/
	}

	if(encoder_results[ENCODER_RIGHT]!=0)
	{
		keys_or_encoders_recently = ENCODERS_RECENTLY;
		last_ui_event = UI_EVENT_ENCODER;


		if(!menu_function && !shift_pressed && !set_pressed && !seq_running && !arpeggiator)
		{
			animate_encoder(ENCODER_RIGHT,LEDS_BLUE2);
		}

		if(samples_since_last_enc_event[ENCODER_RIGHT] > ENC_SPEED_SLOW)
		{
			encoder_speed[ENCODER_RIGHT] = ENC_SPEED_SLOW;
		}

		else if(samples_since_last_enc_event[ENCODER_RIGHT] > ENC_SPEED_NORMAL)
		{
			encoder_speed[ENCODER_RIGHT] = ENC_SPEED_NORMAL;
		}
		else if(samples_since_last_enc_event[ENCODER_RIGHT] < ENC_SPEED_FAST)
		{
			encoder_speed[ENCODER_RIGHT] = ENC_SPEED_FAST;
		}

		//printf("samples_since_last_enc_event[ENCODER_RIGHT]:%d, speed=%s\n", samples_since_last_enc_event[ENCODER_RIGHT],
		//		(encoder_speed[ENCODER_RIGHT]==ENC_SPEED_NORMAL?"NORMAL":"FAST"));

		samples_since_last_enc_event[ENCODER_RIGHT] = 0;

		//printf("~L:%d/%d\n",leds_glowing[0],leds_glowing[1]);

		if(menu_function!=MENU_PLAY || seq_running || arpeggiator)
		{
			led_indication_refresh = 0; //in case menu is active
		}
		//----------------------------------------------------------------------------------------------------------------

		if(enc_function==ENC_FUNCTION_ADJUST_TEMPO) //adjust tempo - right controller multiplies or divides tempo by 2
		{
			if(encoder_results[ENCODER_RIGHT]<=-1)
			{
				SEQUENCER_TIMING *=  2;
				enc_tempo_mult--;
				if(enc_tempo_mult<-5) enc_tempo_mult = -5;
			}
			else if(encoder_results[ENCODER_RIGHT]>=1)
			{
				SEQUENCER_TIMING /=  2;
				enc_tempo_mult++;
				if(enc_tempo_mult>5) enc_tempo_mult = 5;
			}

			if(SEQUENCER_TIMING<SEQUENCER_TIMING_MIN)
			{
				SEQUENCER_TIMING = SEQUENCER_TIMING_MIN;
			}

			#ifdef DEBUG_OUTPUT
			printf("process_encoders(): SEQUENCER_TIMING => %d\n", SEQUENCER_TIMING);
			#endif

			//LEDS_BLUE_OFF;
			if(enc_tempo_mult>0)
			{
				LEDS_BLUE_GLOW(4+enc_tempo_mult)
			}
			else if(enc_tempo_mult<0)
			{
				LEDS_BLUE_GLOW(5+enc_tempo_mult)
			}
			else
			{
				LEDS_BLUE_OFF;
			}


			led_indication_refresh = -2; //block LED refresh until shift released

			//do not allow any further processing of encoder events
			encoder_results[ENCODER_LEFT] = 0;
			encoder_results[ENCODER_RIGHT] = 0;
			return; //to not process anything further in this function
		}

		if(menu_function==MENU_SEQ && sequencer>0) //adjust sequencer steps count
		{
			seq_steps_r2[sequencer-1] += encoder_results[ENCODER_RIGHT];
			if(seq_steps_r2[sequencer-1]<1) { seq_steps_r2[sequencer-1] = 1; }
			if(seq_steps_r2[sequencer-1]>8) { seq_steps_r2[sequencer-1] = 8; }

			sequencer_steps[sequencer-1]=seq_steps_r1[sequencer-1]*seq_steps_r2[sequencer-1];
			#ifdef DEBUG_OUTPUT
			printf("process_encoders(): sequencer_steps[%d] => %d (%dx%d)\n", sequencer-1, sequencer_steps[sequencer-1], seq_steps_r1[sequencer-1], seq_steps_r2[sequencer-1]);
			#endif
			indicate_sequencer_steps(seq_steps_r1[sequencer-1], seq_steps_r2[sequencer-1]);
			seq_running = 0;

			//led_indication_refresh = 0; //in case menu is active
		}

		if(menu_function==KEY_SET_HELD || menu_function==KEY_SHIFT_HELD)
		{
			change_bank(encoder_results[ENCODER_RIGHT]);
			#ifdef DEBUG_OUTPUT
			printf("process_encoders(): bank -> %d\n", bank);
			printf("process_encoders(): find_saved_patches(): saved_patches = %x\n", saved_patches);
			#endif
		}
	}

	//do not allow any further processing of encoder events
	if(enc_function==ENC_FUNCTION_ADJUST_TEMPO)
	{
		encoder_results[ENCODER_LEFT] = 0;
		encoder_results[ENCODER_RIGHT] = 0;
		return; //to not process anything further in this function
	}

	//if(IS_MENU_PLAY_OR_SETTINGS && arpeggiator && encoder_results[ENCODER_LEFT]!=0)
	if((layer==0 || IS_WAVETABLE_LAYER) && (menu_function==MENU_PLAY || menu_function==MENU_ARP) && arpeggiator && ARP_LAYER_MATCHING && encoder_results[ENCODER_LEFT]!=0)
	{
		if(!shift_pressed) //SHIFT adjusts tempo
		{
			//adjust repeats
			arp_repeat += encoder_results[ENCODER_LEFT];
			if(arp_repeat<0) { arp_repeat = 0; }
			if(arp_repeat>ARP_REPEAT_MAX) { arp_repeat = ARP_REPEAT_MAX; }
			//printf("LEDS_BLUE_GLOW(%d);\n", arp_repeat);
			LEDS_BLUE_GLOW(arp_repeat);
			led_indication_refresh = 0;
			#ifdef DEBUG_OUTPUT
			printf("MENU_PLAY:ENCODER_LEFT && arpeggiator: arp_range=%d-%d, arp_octave=%d, arp_repeat=%d\n", arp_range_from, arp_range_to, arp_octave, arp_repeat);
			#endif
		}
	}
	else if((layer==0 || IS_WAVETABLE_LAYER) && (menu_function==MENU_PLAY || menu_function==MENU_SEQ) && sequencer && SEQ_LAYER_MATCHING && seq_running && encoder_results[ENCODER_LEFT]!=0)
	{
		if(!shift_pressed) //SHIFT adjusts tempo
		{
			//adjust seq_octave_mode
			seq_octave_mode += encoder_results[ENCODER_LEFT];
			if(seq_octave_mode<0) { seq_octave_mode = 0; }
			if(seq_octave_mode>SEQ_OCTAVE_MODES_MAX) { seq_octave_mode = SEQ_OCTAVE_MODES_MAX; }
			//printf("LEDS_BLUE_GLOW(%d);\n", seq_octave_mode+1);
			LEDS_BLUE_GLOW(seq_octave_mode+1);
			led_indication_refresh = 0;
			#ifdef DEBUG_OUTPUT
			printf("MENU_PLAY:ENCODER_LEFT && sequencer: seq_octave_mode=%d\n", seq_octave_mode);
			#endif
		}
	}
	//else if(IS_MENU_PLAY_OR_SETTINGS && /*!sequencer &&*/ encoder_results[ENCODER_LEFT]!=0)
	else if(menu_function==MENU_PLAY && /*!sequencer &&*/ encoder_results[ENCODER_LEFT]!=0)
	{
		if(layer==0) //bytebeat tuning
		{
			bytebeat_song_start += encoder_results[ENCODER_LEFT] * (encoder_speed[ENCODER_LEFT]==ENC_SPEED_FAST?BYTEBEAT_START_STEP_FAST:(encoder_speed[ENCODER_LEFT]==ENC_SPEED_SLOW?BYTEBEAT_START_STEP_SLOW:BYTEBEAT_START_STEP_NORMAL));

			//if(bytebeat_song_start > bytebeat_song_length - BYTEBEAT_START_STEP)
			//{
			//	bytebeat_song_start = bytebeat_song_length - BYTEBEAT_START_STEP;
			//}
			if(bytebeat_song_start < 0)
			{
				bytebeat_song_start = 0;
			}
			if(set_pressed && patch!=PATCH_SILENCE && (!arpeggiator || ARP_LAYER_DIFFERENT) && (!sequencer || SEQ_LAYER_DIFFERENT)) //update the patch parameters too
			{
				patch_song_start[patch] = bytebeat_song_start;
			}
			#ifdef DEBUG_OUTPUT
			printf("MENU_PLAY:ENCODER_LEFT start=%d,len=%d,s=%s\n", bytebeat_song_start, bytebeat_song_length, encoder_speed[ENCODER_LEFT]==ENC_SPEED_FAST?"FAST":(encoder_speed[ENCODER_LEFT]==ENC_SPEED_SLOW?"SLOW":"NORMAL"));
			#endif
		}
		else if(IS_WAVETABLE_LAYER)
		{
			if(last_note_triggered)
			{
				if(set_pressed) //change waveform
				{
					mini_piano_tuning_enc = mini_piano_waves[last_note_triggered-1];
					mini_piano_tuning_enc += encoder_results[ENCODER_LEFT];
					if(mini_piano_tuning_enc < MINI_PIANO_WAVE_FIRST) { mini_piano_tuning_enc = MINI_PIANO_WAVE_LAST; }
					if(mini_piano_tuning_enc > MINI_PIANO_WAVE_LAST) { mini_piano_tuning_enc = MINI_PIANO_WAVE_FIRST; }
					mini_piano_waves[last_note_triggered-1] = mini_piano_tuning_enc;
					note_triggered = last_note_triggered;

					LEDS_BLUE_GLOW(mini_piano_tuning_enc+1);
					led_indication_refresh = 0;
				}
				else //octave up or down
				{
					mini_piano_tuning_enc = mini_piano_tuning[last_note_triggered-1];

					//random noise waveform, LPF applied instead of tuning, left encoder changes envelope decay speed
					if(mini_piano_waves[last_note_triggered-1]==MINI_PIANO_WAVE_RNG
					|| (mini_piano_waves[last_note_triggered-1]==MINI_PIANO_WAVE_DEFAULT && layer==MINI_PIANO_LAYER_RNG))
					{
						rnd_envelope_div[last_note_triggered-1] -= encoder_results[ENCODER_LEFT];
						if(rnd_envelope_div[last_note_triggered-1]<1) { rnd_envelope_div[last_note_triggered-1] = 1; }
						if(rnd_envelope_div[last_note_triggered-1]>WAVE_RNG_DIVIDER_MAX) { rnd_envelope_div[last_note_triggered-1] = WAVE_RNG_DIVIDER_MAX; }
						#ifdef DEBUG_OUTPUT
						printf("MENU_PLAY:ENCODER_LEFT last_note_triggered = %d, rnd_envelope_div[last_note_triggered-1] = %d\n", last_note_triggered, rnd_envelope_div[last_note_triggered-1]);
						#endif
					}
					else //all other waveforms that are directly tunable and work with octaves
					{
						if(encoder_results[ENCODER_LEFT]>=1)
						{
							if(mini_piano_tuning_enc*2 <= MINI_PIANO_TUNING_MAX)
							{
								mini_piano_tuning_enc *= 2;
							}
						}
						else if(encoder_results[ENCODER_LEFT]<=-1)
						{
							if(mini_piano_tuning_enc/2 >= MINI_PIANO_TUNING_MIN_OCTAVE)
							{
								mini_piano_tuning_enc /= 2;
							}
						}

						//if(mini_piano_tuning_enc < MINI_PIANO_TUNING_MIN) { mini_piano_tuning_enc = MINI_PIANO_TUNING_MIN; }
						//if(mini_piano_tuning_enc > MINI_PIANO_TUNING_MAX) { mini_piano_tuning_enc = MINI_PIANO_TUNING_MAX; }
						mini_piano_tuning[last_note_triggered-1] = mini_piano_tuning_enc;
					}

					note_triggered = last_note_triggered;
				}
				#ifdef DEBUG_OUTPUT
				printf("MENU_PLAY:ENCODER_LEFT last_note_triggered = %d, mini_piano_tuning[last_note_triggered-1] = %d, mini_piano_waves[last_note_triggered-1] = %d\n", last_note_triggered, mini_piano_tuning[last_note_triggered-1], mini_piano_waves[last_note_triggered-1]);
				#endif
			}
			encoder_results[ENCODER_LEFT] = 0;
		}
		if(IS_FLASH_SAMPLE_LAYER) //flash sample selector
		{
			if(layer7_last_key >= 0)
			{
				layer7_patch_start[layer7_last_key] += encoder_results[ENCODER_LEFT] * (encoder_speed[ENCODER_LEFT]==ENC_SPEED_FAST?BYTEBEAT_START_STEP_FAST:(encoder_speed[ENCODER_LEFT]==ENC_SPEED_SLOW?BYTEBEAT_START_STEP_SLOW:BYTEBEAT_START_STEP_NORMAL));

				if(layer7_patch_start[layer7_last_key] < 0)
				{
					layer7_patch_start[layer7_last_key] = 0;
				}

				if(layer7_patch_start[layer7_last_key] > SAMPLES_LENGTH - FLASH_SAMPLE_LENGTH_MIN)
				{
					layer7_patch_start[layer7_last_key] = SAMPLES_LENGTH - FLASH_SAMPLE_LENGTH_MIN;
				}

				layer7_playhead = layer7_patch_start[layer7_last_key];
				layer7_grain_length = layer7_patch_length[layer7_last_key];
				layer7_grain_bitrate = layer7_patch_bitrate[layer7_last_key];
				layer7_patch_modified = 1;

				#ifdef DEBUG_OUTPUT
				printf("MENU_PLAY:ENCODER_LEFT layer7_last_key=%d, start=%d, len=%d, s=%s\n", layer7_last_key, layer7_patch_start[layer7_last_key], layer7_patch_length[layer7_last_key], encoder_speed[ENCODER_LEFT]==ENC_SPEED_FAST?"FAST":(encoder_speed[ENCODER_LEFT]==ENC_SPEED_SLOW?"SLOW":"NORMAL"));
				#endif
			}
		}
	}

	//if(IS_MENU_PLAY_OR_SETTINGS && arpeggiator && encoder_results[ENCODER_RIGHT]!=0)
	if((layer==0 || IS_WAVETABLE_LAYER) && (menu_function==MENU_PLAY || menu_function==MENU_ARP) && arpeggiator && ARP_LAYER_MATCHING && encoder_results[ENCODER_RIGHT]!=0)
	{
		//adjust range
		arp_octave += encoder_results[ENCODER_RIGHT];
		if(arp_octave<ARP_OCTAVE_RANGE_MIN) { arp_octave = ARP_OCTAVE_RANGE_MIN; arp_octave_effective = arp_octave; }
		if(arp_octave>ARP_OCTAVE_RANGE_MAX) { arp_octave = ARP_OCTAVE_RANGE_MAX; arp_octave_effective = arp_octave; }
		if(arp_range_from>arp_octave || set_pressed) { arp_range_from = arp_octave; arp_octave_effective = arp_octave; }
		if(arp_range_to<arp_octave || set_pressed) { arp_range_to = arp_octave; arp_octave_effective = arp_octave; }
		if(shift_pressed && arp_octave > 0) { arp_range_from = 0; arp_range_to = arp_octave; arp_octave_effective = arp_octave; }
		if(shift_pressed && arp_octave < 0) { arp_range_to = 0; arp_range_from = arp_octave; arp_octave_effective = arp_octave; }
		//printf("LEDS_BLUE_GLOW(%d);\n", (arp_octave<5)?(arp_octave+3):8);
		if(arp_octave>4)
		{
			LEDS_BLUE_BLINK((5+(arp_octave+3)%4)); //extra parentheses here for macro
		}
		else
		{
			LEDS_BLUE_GLOW(((arp_octave<5)?(arp_octave+4):8)); //extra parentheses here for macro
		}
		led_indication_refresh = 0;
		#ifdef DEBUG_OUTPUT
		printf("MENU_PLAY:ENCODER_RIGHT && arpeggiator: arp_range=%d-%d, arp_octave=%d, arp_repeat=%d\n", arp_range_from, arp_range_to, arp_octave, arp_repeat);
		#endif
	}
	else if((layer==0 || IS_WAVETABLE_LAYER) && (menu_function==MENU_PLAY || menu_function==MENU_SEQ) && sequencer && SEQ_LAYER_MATCHING && seq_running && encoder_results[ENCODER_RIGHT]!=0)
	{
		//adjust range
		seq_octave += encoder_results[ENCODER_RIGHT];
		if(seq_octave<SEQ_OCTAVE_RANGE_MIN) { seq_octave = SEQ_OCTAVE_RANGE_MIN; seq_octave_effective = seq_octave; }
		if(seq_octave>SEQ_OCTAVE_RANGE_MAX) { seq_octave = SEQ_OCTAVE_RANGE_MAX; seq_octave_effective = seq_octave; }
		if(seq_range_from>seq_octave || set_pressed) { seq_range_from = seq_octave; seq_octave_effective = seq_octave; }
		if(seq_range_to<seq_octave || set_pressed) { seq_range_to = seq_octave; seq_octave_effective = seq_octave; }
		if(shift_pressed && seq_octave > 0) { seq_range_from = 0; seq_range_to = seq_octave; seq_octave_effective = seq_octave; }
		if(shift_pressed && seq_octave < 0) { seq_range_to = 0; seq_range_from = seq_octave; seq_octave_effective = seq_octave; }

		//printf("LEDS_BLUE_GLOW(%d);\n", (seq_octave<5)?(seq_octave+3):8);
		if(seq_octave>4)
		{
			LEDS_BLUE_BLINK((5+(seq_octave+3)%4)); //extra parentheses here for macro
		}
		else
		{
			LEDS_BLUE_GLOW(((seq_octave<5)?(seq_octave+4):8)); //extra parentheses here for macro
		}
		led_indication_refresh = 0;
		#ifdef DEBUG_OUTPUT
		printf("MENU_PLAY:ENCODER_RIGHT && sequencer: seq_range=%d-%d, seq_octave=%d, seq_octave_mode=%d\n", seq_range_from, seq_range_to, seq_octave, seq_octave_mode);
		#endif
	}
	//else if(IS_MENU_PLAY_OR_SETTINGS && /*!sequencer &&*/ encoder_results[ENCODER_RIGHT]!=0)
	else if(menu_function==MENU_PLAY && /*!sequencer &&*/ encoder_results[ENCODER_RIGHT]!=0)
	{
		if(encoder_speed[ENCODER_RIGHT]==ENC_SPEED_FAST)
		{
			encoder_speed_multiplier = 2.0f;
		}
		else if(encoder_speed[ENCODER_RIGHT]==ENC_SPEED_SLOW)
		{
			encoder_speed_multiplier = 0.5f;
		}
		else
		{
			encoder_speed_multiplier = 1.0f;
		}

		if(layer==0) //bytebeat tuning
		{
			if(bytebeat_song_length==BYTEBEAT_LENGTH_UNLIMITED && encoder_results[ENCODER_RIGHT]<0)
			{
				bytebeat_song_length = BYTEBEAT_LENGTH_DEFAULT;
			}
			else if(bytebeat_song_length!=BYTEBEAT_LENGTH_UNLIMITED)
			{
				//3 different ranges for variable step length
				if(bytebeat_song_length<BYTEBEAT_LENGTH_THR_FINE)
				{
					bytebeat_song_length += encoder_results[ENCODER_RIGHT] * (int)ceil((float)bytebeat_song_length/(float)BYTEBEAT_LENGTH_STEP_FINE*encoder_speed_multiplier);
				}
				else if(bytebeat_song_length<BYTEBEAT_LENGTH_THR_COARSE)
				{
					bytebeat_song_length += encoder_results[ENCODER_RIGHT] * (int)ceil((float)bytebeat_song_length/(float)BYTEBEAT_LENGTH_STEP*encoder_speed_multiplier);
				}
				else
				{
					bytebeat_song_length += encoder_results[ENCODER_RIGHT] * (int)ceil((float)bytebeat_song_length/(float)BYTEBEAT_LENGTH_STEP_COARSE*encoder_speed_multiplier);
				}

				if(bytebeat_song_length < BYTEBEAT_LENGTH_MIN)
				{
					bytebeat_song_length = BYTEBEAT_LENGTH_MIN;
				}

				if(bytebeat_song_length > BYTEBEAT_LENGTH_MAX)
				{
					bytebeat_song_length = BYTEBEAT_LENGTH_UNLIMITED;
				}
			}

			encoder_results[ENCODER_RIGHT] = 0;

			if(set_pressed && patch!=PATCH_SILENCE && (!arpeggiator || ARP_LAYER_DIFFERENT) && (!sequencer || SEQ_LAYER_DIFFERENT)) //update the patch parameters too
			{
				patch_song_length[patch] = bytebeat_song_length;
			}
			#ifdef DEBUG_OUTPUT
			printf("MENU_PLAY:ENCODER_RIGHT start=%d,len=%d,s=%s\n", bytebeat_song_start, bytebeat_song_length, encoder_speed[ENCODER_RIGHT]==ENC_SPEED_FAST?"FAST":(encoder_speed[ENCODER_RIGHT]==ENC_SPEED_SLOW?"SLOW":"NORMAL"));
			#endif
		}
		else if(IS_WAVETABLE_LAYER)
		{
			if(last_note_triggered)
			{
				mini_piano_tuning_enc = mini_piano_tuning[last_note_triggered-1];

				if(mini_piano_waves[last_note_triggered-1]==MINI_PIANO_WAVE_RNG
				|| (mini_piano_waves[last_note_triggered-1]==MINI_PIANO_WAVE_DEFAULT && layer==MINI_PIANO_LAYER_RNG))
				{
					mini_piano_tuning_enc += encoder_results[ENCODER_RIGHT] * (mini_piano_tuning_enc / 5 + 1) * encoder_speed_multiplier;
					if(mini_piano_tuning_enc < MINI_PIANO_TUNING_MIN_RNG) { mini_piano_tuning_enc = MINI_PIANO_TUNING_MIN_RNG; } //standard low end does not produce sound anymore
					if(mini_piano_tuning_enc > MINI_PIANO_TUNING_MAX) { mini_piano_tuning_enc = MINI_PIANO_TUNING_MAX; }
				}
				else
				{
					mini_piano_tuning_enc += encoder_results[ENCODER_RIGHT] * (mini_piano_tuning_enc / 100 + 1) * encoder_speed_multiplier;
					if(mini_piano_tuning_enc < MINI_PIANO_TUNING_MIN) { mini_piano_tuning_enc = MINI_PIANO_TUNING_MIN; }
					if(mini_piano_tuning_enc > MINI_PIANO_TUNING_MAX) { mini_piano_tuning_enc = MINI_PIANO_TUNING_MAX; }
				}

				mini_piano_tuning[last_note_triggered-1] = mini_piano_tuning_enc;
			}

			encoder_results[ENCODER_RIGHT] = 0;
			#ifdef DEBUG_OUTPUT
			printf("MENU_PLAY:ENCODER_RIGHT last_note_triggered = %d, mini_piano_tuning[last_note_triggered-1] = %d\n", last_note_triggered, mini_piano_tuning[last_note_triggered-1]);
			#endif
		}
		else if(IS_FLASH_SAMPLE_LAYER) //flash sample selector
		{
			if(layer7_last_key >= 0)
			{
				layer7_patch_length[layer7_last_key] += encoder_results[ENCODER_RIGHT] * (encoder_speed[ENCODER_RIGHT]==ENC_SPEED_FAST?BYTEBEAT_START_STEP_FAST:(encoder_speed[ENCODER_RIGHT]==ENC_SPEED_SLOW?BYTEBEAT_START_STEP_SLOW:BYTEBEAT_START_STEP_NORMAL));

				if(layer7_patch_length[layer7_last_key] < FLASH_SAMPLE_LENGTH_MIN)
				{
					layer7_patch_length[layer7_last_key] = FLASH_SAMPLE_LENGTH_MIN;
				}

				if(layer7_patch_length[layer7_last_key] > SAMPLES_LENGTH)
				{
					layer7_patch_length[layer7_last_key] = SAMPLES_LENGTH;
				}

				layer7_playhead = layer7_patch_start[layer7_last_key];
				layer7_grain_length = layer7_patch_length[layer7_last_key];
				layer7_grain_bitrate = layer7_patch_bitrate[layer7_last_key];
				layer7_patch_modified = 1;

				//only play end bit of the grain, for easier navigation
				if(layer7_grain_bitrate>=0)
				{
					if(layer7_grain_length>FLASH_SAMPLE_RESIZING_PREVIEW)
					{
						layer7_playhead += layer7_grain_length - FLASH_SAMPLE_RESIZING_PREVIEW;
						layer7_grain_length = FLASH_SAMPLE_RESIZING_PREVIEW;
					}
				}
				else
				{
					if(layer7_grain_length > (FLASH_SAMPLE_RESIZING_PREVIEW*(1-layer7_grain_bitrate)))
					{
						layer7_playhead += layer7_grain_length - (FLASH_SAMPLE_RESIZING_PREVIEW*(1-layer7_grain_bitrate));
						layer7_grain_length = FLASH_SAMPLE_RESIZING_PREVIEW*(1-layer7_grain_bitrate);
					}
				}

				#ifdef DEBUG_OUTPUT
				printf("MENU_PLAY:ENCODER_RIGHT layer7_last_key=%d, start=%d, len=%d, s=%s\n", layer7_last_key, layer7_patch_start[layer7_last_key], layer7_patch_length[layer7_last_key], encoder_speed[ENCODER_RIGHT]==ENC_SPEED_FAST?"FAST":(encoder_speed[ENCODER_RIGHT]==ENC_SPEED_SLOW?"SLOW":"NORMAL"));
				#endif
			}
		}
	}

	if(menu_function==MENU_FRM && encoder_results[ENCODER_LEFT]!=0)
	{
		bytebeat_song += encoder_results[ENCODER_LEFT];

		/*
		bytebeat_song_ptr = 0;
		//bytebeat_speed_div = 0;
		bytebeat_song_length=BYTEBEAT_LENGTH_UNLIMITED;

		sampleCounter = 0;
		sequencer = 0;
		*/

		if(bytebeat_song>=BYTEBEAT_SONGS)
		{
			bytebeat_song = 0;
		}

		if(bytebeat_song<0)
		{
			bytebeat_song = BYTEBEAT_SONGS-1;
		}

		LEDS_BLUE_GLOW(bytebeat_song+1);

		#ifdef DEBUG_OUTPUT
		printf("MENU_FRM:ENCODER_LEFT song=%d\n",bytebeat_song);
		#endif
	}
	/*
	if(menu_function==MENU_ARP && encoder_results[ENCODER_LEFT]!=0)
	{
		arpeggiator += encoder_results[ENCODER_LEFT];

		if(arpeggiator>ARP_MAX)
		{
			arpeggiator = 1;
		}

		if(arpeggiator<=0)
		{
			arpeggiator = ARP_MAX;
		}

		LEDS_BLUE_GLOW(arpeggiator);

		#ifdef DEBUG_OUTPUT
		printf("MENU_ARP:ENCODER_LEFT arpeggiator=%d\n",arpeggiator);
		#endif
	}
 	*/
	if(menu_function==MENU_DEL && encoder_results[ENCODER_RIGHT]!=0)
	{
		echo_dynamic_loop_current_step += encoder_results[ENCODER_RIGHT];
		if(echo_dynamic_loop_current_step < 0) { echo_dynamic_loop_current_step = 0; }
		if(echo_dynamic_loop_current_step >= ECHO_DYNAMIC_LOOP_STEPS) { echo_dynamic_loop_current_step = ECHO_DYNAMIC_LOOP_STEPS - 1; }

		echo_dynamic_loop_length = echo_dynamic_loop_steps[echo_dynamic_loop_current_step];
		bytebeat_echo_on = (echo_dynamic_loop_length>0);

		LEDS_BLUE_GLOW(echo_dynamic_loop_current_step+1);
		#ifdef DEBUG_OUTPUT
		printf("MENU_DEL:ENCODER_RIGHT bytebeat_echo_on => %d, echo_dynamic_loop_length => %d\n", bytebeat_echo_on, echo_dynamic_loop_length);
		#endif
	}

	if(menu_function==MENU_DEL && encoder_results[ENCODER_LEFT]!=0)
	{
		/*
		if(encoder_speed[ENCODER_LEFT]==ENC_SPEED_SLOW)
		{
			echo_dynamic_loop_length -= encoder_results[ENCODER_LEFT] * 10;
		}
		else*/ if(encoder_speed[ENCODER_LEFT]==ENC_SPEED_FAST)
		{
			//echo_dynamic_loop_length -= encoder_results[ENCODER_LEFT] * 1000;
			echo_dynamic_loop_length -= encoder_results[ENCODER_LEFT] * (echo_dynamic_loop_length/10+10);
		}
		else
		{
			//echo_dynamic_loop_length -= encoder_results[ENCODER_LEFT] * 100;
			echo_dynamic_loop_length -= encoder_results[ENCODER_LEFT] * (echo_dynamic_loop_length/100+1);
		}


		if(echo_dynamic_loop_length > ECHO_DYNAMIC_LOOP_LENGTH_MAX)
		{
			echo_dynamic_loop_length = ECHO_DYNAMIC_LOOP_LENGTH_MAX;
		}
		if(echo_dynamic_loop_length < ECHO_DYNAMIC_LOOP_LENGTH_MIN)
		{
			echo_dynamic_loop_length = ECHO_DYNAMIC_LOOP_LENGTH_MIN;
		}

		//echo_dynamic_loop_current_step = 0;
		bytebeat_echo_on = 1;

		//LEDS_BLUE_GLOW(echo_dynamic_loop_current_step+1);
		#ifdef DEBUG_OUTPUT
		printf("MENU_DEL:ENCODER_LEFT bytebeat_echo_on => %d, echo_dynamic_loop_length => %d\n", bytebeat_echo_on, echo_dynamic_loop_length);
		#endif
	}

	if(menu_function==MENU_VOL && encoder_results[ENCODER_RIGHT]!=0)
	{
		if(layer==0)
		{
			if(encoder_results[ENCODER_RIGHT]>0)
			{
				if(BB_SHIFT_VOLUME<BB_VOLUME_MAX)
				{
					BB_SHIFT_VOLUME++;
					//printf("BB_SHIFT_VOLUME++;BB_SHIFT_VOLUME=%d\n", BB_SHIFT_VOLUME);
				}
			}
			else
			{
				if(BB_SHIFT_VOLUME>BB_VOLUME_MIN)
				{
					BB_SHIFT_VOLUME--;
					//printf("BB_SHIFT_VOLUME--;BB_SHIFT_VOLUME=%d\n", BB_SHIFT_VOLUME);
				}
			}
			LEDS_BLUE_GLOW(BB_SHIFT_VOLUME+1);
		}
		else if(IS_WAVETABLE_LAYER)
		{
			if(encoder_results[ENCODER_RIGHT]>0)
			{
				if(WAVESAMPLE_BOOST_VOLUME<WAVESAMPLE_BOOST_VOLUME_MAX)
				{
					WAVESAMPLE_BOOST_VOLUME++;
					//printf("WAVESAMPLE_BOOST_VOLUME++;WAVESAMPLE_BOOST_VOLUME=%d\n", WAVESAMPLE_BOOST_VOLUME);
				}
			}
			else
			{
				if(WAVESAMPLE_BOOST_VOLUME>WAVESAMPLE_BOOST_VOLUME_MIN)
				{
					WAVESAMPLE_BOOST_VOLUME--;
					//printf("WAVESAMPLE_BOOST_VOLUME--;WAVESAMPLE_BOOST_VOLUME=%d\n", WAVESAMPLE_BOOST_VOLUME);
				}
			}
			LEDS_BLUE_GLOW(WAVESAMPLE_BOOST_VOLUME-WAVESAMPLE_BOOST_VOLUME_MIN+1);
		}
		else if(IS_FLASH_SAMPLE_LAYER)
		{
			if(encoder_results[ENCODER_RIGHT]>0)
			{
				if(FLASH_SAMPLE_BOOST_VOLUME<FLASH_SAMPLE_BOOST_VOLUME_MAX)
				{
					FLASH_SAMPLE_BOOST_VOLUME++;
					//printf("FLASH_SAMPLE_BOOST_VOLUME++;FLASH_SAMPLE_BOOST_VOLUME=%d\n", FLASH_SAMPLE_BOOST_VOLUME);
				}
			}
			else
			{
				if(FLASH_SAMPLE_BOOST_VOLUME>FLASH_SAMPLE_BOOST_VOLUME_MIN)
				{
					FLASH_SAMPLE_BOOST_VOLUME--;
					//printf("FLASH_SAMPLE_BOOST_VOLUME--;FLASH_SAMPLE_BOOST_VOLUME=%d\n", FLASH_SAMPLE_BOOST_VOLUME);
				}
			}
			LEDS_BLUE_GLOW(FLASH_SAMPLE_BOOST_VOLUME-FLASH_SAMPLE_BOOST_VOLUME_MIN+1);
		}
		#ifdef DEBUG_OUTPUT
		printf("MENU_VOL:ENCODER_RIGHT BB_SHIFT_VOLUME=%d, WAVESAMPLE_BOOST_VOLUME=%d, FLASH_SAMPLE_BOOST_VOLUME=%d\n", BB_SHIFT_VOLUME, WAVESAMPLE_BOOST_VOLUME, FLASH_SAMPLE_BOOST_VOLUME);
		#endif
	}

	if(menu_function==MENU_BIT && encoder_results[ENCODER_LEFT]!=0)
	{
		if(layer==0)
		{
			//if(encoder_results[ENCODER_LEFT]<0) bit2=4;
			//if(encoder_results[ENCODER_LEFT]>0) bit2=2;
			//LEDS_BLUE_GLOW(bit2);

			if(bit2>1 && encoder_results[ENCODER_LEFT]<0) bit2--;
			if(bit2<8 && encoder_results[ENCODER_LEFT]>0) bit2++;
			LEDS_BLUE_GLOW(bit2);

			#ifdef DEBUG_OUTPUT
			printf("MENU_BIT:ENCODER_LEFT bit1 => %d, bit2 => %d\n", bit1, bit2);
			#endif
		}
	}

	if(menu_function==MENU_BIT && encoder_results[ENCODER_RIGHT]!=0)
	{
		if(layer==0)
		{
			//if(bit2>1 && encoder_results[ENCODER_RIGHT]<0) bit2--;
			//if(bit2<8 && encoder_results[ENCODER_RIGHT]>0) bit2++;
			//LEDS_BLUE_GLOW(bit2);

			if(bit1>1 && encoder_results[ENCODER_RIGHT]<0) bit1--;
			if(bit1<8 && encoder_results[ENCODER_RIGHT]>0) bit1++;
			LEDS_BLUE_GLOW(bit1);
			#ifdef DEBUG_OUTPUT
			printf("MENU_BIT:ENCODER_RIGHT bit1 => %d, bit2 => %d\n", bit1, bit2);
			#endif
		}
		else if(IS_FLASH_SAMPLE_LAYER)
		{
			if(layer7_last_key >= 0)
			{
				if(layer7_patch_bitrate[layer7_last_key]>FLASH_SAMPLE_BITRATE_MIN && encoder_results[ENCODER_RIGHT]>0)
				{
					layer7_patch_bitrate[layer7_last_key]--;
					if(layer7_patch_bitrate[layer7_last_key]==7) layer7_patch_bitrate[layer7_last_key] = 6; //skip "7" as divider value
				}
				if(layer7_patch_bitrate[layer7_last_key]<FLASH_SAMPLE_BITRATE_MAX && encoder_results[ENCODER_RIGHT]<0)
				{
					layer7_patch_bitrate[layer7_last_key]++;
					if(layer7_patch_bitrate[layer7_last_key]==7) layer7_patch_bitrate[layer7_last_key] = 8; //skip "7" as divider value
				}

				layer7_playhead = layer7_patch_start[layer7_last_key];
				layer7_grain_length = layer7_patch_length[layer7_last_key];
				layer7_grain_bitrate = layer7_patch_bitrate[layer7_last_key];
				layer7_patch_modified = 1;

				#ifdef DEBUG_OUTPUT
				printf("MENU_PLAY:ENCODER_RIGHT layer7_last_key=%d, start=%d, len=%d, s=%s\n", layer7_last_key, layer7_patch_start[layer7_last_key], layer7_patch_length[layer7_last_key], encoder_speed[ENCODER_LEFT]==ENC_SPEED_FAST?"FAST":(encoder_speed[ENCODER_LEFT]==ENC_SPEED_SLOW?"SLOW":"NORMAL"));
				#endif

				if(layer7_grain_bitrate>=0)
				{
					LEDS_BLUE_GLOW(layer7_grain_bitrate==8?1:8-layer7_grain_bitrate);
				}
				else
				{
					LEDS_BLUE_BLINK(5+((3-layer7_grain_bitrate)%4));
				}
			}
		}
	}

	if(menu_function==MENU_VAR && encoder_results[ENCODER_RIGHT]!=0 && layer==0)
	{
		var_p[variable_n]+=encoder_results[ENCODER_RIGHT];

		//LEDS_BLUE_GLOW(variable_n);
		animate_encoder(ENCODER_RIGHT,LEDS_BLUE2);
		#ifdef DEBUG_OUTPUT
		printf("MENU_VAR:ENCODER_RIGHT var_p[%d]=%d\n", variable_n, var_p[variable_n]);
		#endif
	}

	if(menu_function==MENU_VAR && encoder_results[ENCODER_LEFT]!=0 && layer==0)
	{
		var_p[variable_n]+=encoder_results[ENCODER_LEFT] * 16;

		//LEDS_BLUE_GLOW(variable_n);
		animate_encoder(ENCODER_LEFT,LEDS_BLUE1);
		#ifdef DEBUG_OUTPUT
		printf("MENU_VAR:ENCODER_LEFT var_p[%d]=%d\n", variable_n, var_p[variable_n]);
		#endif
	}

	//sensor settings menu
	if(menu_function==MENU_SETTINGS && encoder_results[ENCODER_RIGHT]!=0)
	{
		if(sensor_range>SENSOR_RANGE_DEFAULT*2)
		{
			sensor_range -= encoder_results[ENCODER_RIGHT] * (sensor_range/10);
		}
		else
		{
			sensor_range -= encoder_results[ENCODER_RIGHT] * (sensor_range/100);
		}
		if(sensor_range<SENSOR_RANGE_MIN) { sensor_range = SENSOR_RANGE_MIN; }
		if(sensor_range>SENSOR_RANGE_MAX) { sensor_range = SENSOR_RANGE_MAX; }

		//LEDS_BLUE_GLOW(variable_n);
		//animate_encoder(ENCODER_RIGHT,LEDS_BLUE2);
		indicate_sensors_base_range();
		#ifdef DEBUG_OUTPUT
		printf("MENU_SETTINGS:ENCODER_RIGHT sensor_range => %f\n", sensor_range);
		#endif
	}

	if(menu_function==MENU_SETTINGS && encoder_results[ENCODER_LEFT]!=0)
	{
		if(encoder_speed[ENCODER_LEFT]==ENC_SPEED_SLOW)
		{
			sensor_base -= encoder_results[ENCODER_LEFT] * 10;
		}
		else if(encoder_speed[ENCODER_LEFT]==ENC_SPEED_FAST)
		{
			sensor_base -= encoder_results[ENCODER_LEFT] * (sensor_base/10+1);
		}
		else
		{
			sensor_base -= encoder_results[ENCODER_LEFT] * (sensor_base/50+1);
		}
		if(sensor_base<SENSOR_BASE_MIN) { sensor_base = SENSOR_BASE_MIN; }
		if(sensor_base>SENSOR_BASE_MAX) { sensor_base = SENSOR_BASE_MAX; }

		//LEDS_BLUE_GLOW(variable_n);
		//animate_encoder(ENCODER_LEFT,LEDS_BLUE1);
		indicate_sensors_base_range();
		#ifdef DEBUG_OUTPUT
		printf("MENU_SETTINGS:ENCODER_LEFT sensor_base => %d\n", sensor_base);
		#endif
	}

	encoder_results[ENCODER_LEFT] = 0;
	encoder_results[ENCODER_RIGHT] = 0;
}
