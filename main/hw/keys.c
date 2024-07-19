/*
 * keys.c
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 27 Jan 2021
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

#include "keys.h"

#include <init.h>
#include <signals.h>
#include <gpio.h>
#include <settings.h>
#include <dsp/Bytebeat.h>
#include <dsp/SineWaves.h>

//#define DEBUG_OUTPUT
//#define PRINT_PATCH_DATA

void trigger_patch_octave(int k, int octave_div)
{
	if(k==PATCH_SILENCE)
	{
		#ifdef DEBUG_OUTPUT
		printf("trigger_patch_octave(PATCH_SILENCE)\n");
		#endif
		bytebeat_song = -1; //this will effectively not mix in data from any song
		bb_last_note_triggered = -1;
	}
	else
	{
		#ifdef DEBUG_OUTPUT
		printf("trigger_patch_octave(k=%d, div=%d)\n", k, octave_div);
		#endif

		int divider;

		if(octave_div>=0)
		{
			divider = octave_div+1;
		}
		else if(octave_div<=-2)
		{
			divider = -4;
		}
		else
		{
			divider = octave_div-1;
		}

		bytebeat_song_start = patch_song_start[k];

		if(patch_song_length[k]==-1) //unlimited length
		{
			bytebeat_song_length = patch_song_length[k];
		}
		else if(divider>0)
		{
			bytebeat_song_length = patch_song_length[k] / divider;
		}
		else
		{
			bytebeat_song_length = patch_song_length[k] * -divider;
		}

		bytebeat_song = patch_song[k];

		if(!bb_ptr_continuous || bb_last_note_triggered==-1)
		{
			bytebeat_song_ptr = bytebeat_song_start;
		}

		bytebeat_song_length_effective = bytebeat_song_length;
		bytebeat_song_start_effective = bytebeat_song_start;

		bit1 = patch_bit1[k];
		bit2 = patch_bit2[k];

		var_p[0] = patch_var1[k];
		var_p[1] = patch_var2[k];
		var_p[2] = patch_var3[k];
		var_p[3] = patch_var4[k];

		bb_last_note_triggered = k;

		#if defined(DEBUG_OUTPUT) || defined(PRINT_PATCH_DATA)
		//printf("div=%d, s=%d, st/le=%d/%d\n", divider, bytebeat_song, bytebeat_song_start_effective, bytebeat_song_length_effective);
		printf("KEY(%d): f=%d, s/l=%d/%d, b=%d/%d, v=%d/%d/%d/%d\n", k, patch_song[k],
				patch_song_start[k], patch_song_length[k],
				bit1, bit2, var_p[0], var_p[1], var_p[2], var_p[3]);
		#endif
	}
}

void trigger_patch(int k)
{
	trigger_patch_octave(k, 0);
}

void key_pressed(int key)
{
	//printf("key_pressed(%d): led_disp[LEDS_ORANGE]=%d\n",key,led_disp[LEDS_ORANGE]);
	#ifdef DEBUG_OUTPUT
	printf("key_pressed(%d)\n",key);
	#endif

	if(key==KEY_BOTH_HELD)
	{
		//special command
		#ifdef DEBUG_OUTPUT
		printf("key_pressed(%d): KEY_BOTH_HELD -> SERVICE menu\n",key);
		#endif
		menu_function = KEY_BOTH_HELD;
		service_seq = 0;
		service_seq_cnt = 0;
		LEDS_display_direct(0,0);
		trigger_patch(PATCH_SILENCE);
		sine_waves_stop_sound();
		sound_stopped = 1;
		return;
	}

	if(menu_function==KEY_BOTH_HELD)
	{
		if(IS_KEY_EXIT(key))
		{
			if(IS_KEY_SHIFT(key)) //exit
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): exit the SERVICE menu\n", key);
				#endif
				sound_stopped = 0;
			}
			if(IS_KEY_SET(key)) //accept
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): SERVICE menu sequence = %llu ACCEPTED\n", key, service_seq);
				#endif
				if(service_seq==111)
				{
					bb_ptr_continuous = 0;
				}
				if(service_seq==222)
				{
					bb_ptr_continuous = 1;
				}
				if(service_seq==3331)
				{
					show_firmware_version();
				}
				if(service_seq==3332)
				{
					show_mac();
				}
				if(service_seq==3333)
				{
					light_sensors_test();
				}
				if(service_seq==43212288)
				{
					reset_higher_layers(1,1);
				}
				if(service_seq==43212222)
				{
					reset_higher_layers(1,0);
				}
				if(service_seq==43218888)
				{
					reset_higher_layers(0,1);
				}
				if(service_seq==6666)
				{
					MCU_restart(); //for debugging
				}
				if(service_seq==33333331)
				{
					set_sample_format(1);
					FLASH_SAMPLE_FORMAT = get_sample_format();
				}
				if(service_seq==33333332)
				{
					set_sample_format(2);
					FLASH_SAMPLE_FORMAT = get_sample_format();
				}
				if(service_seq==44554455)
				{
					reset_selftest_pass();
				}
				if(service_seq==44445555)
				{
					settings_stats();
				}
				#ifdef ALLOW_BACKUP_PATCHES
				if(service_seq==45454545)
				{
					backup_restore_patches("nvs", "nbkp");
				}
				#endif
				if(service_seq==54545454)
				{
					backup_restore_patches("nbkp", "nvs");
				}
				if(service_seq/10==5555555)
				{
					ENCODER_STEPS_PER_EVENT = service_seq%10;
					set_encoder_steps_per_event(ENCODER_STEPS_PER_EVENT);
					printf("key_pressed(%d): SERVICE menu: ENCODER_STEPS_PER_EVENT => %d\n", key, ENCODER_STEPS_PER_EVENT);
				}
				if(service_seq==88888888)
				{
					delete_last_loaded_patch();
				}
				if(service_seq/10==7777777)
				{
					delete_sequence(service_seq%10);
				}
				if(service_seq==87654321)
				{
					settings_reset();
				}
			}
			menu_function = MENU_EXIT;
			last_ui_event = UI_EVENT_EXIT_MENU;
			LEDS_display_direct_end();
			LEDS_BLUE_OFF;
			sound_stopped = 0;

			//to not detect SET or SHIFT click and exit immediately from this menu
			set_clicked = 0;
			shift_clicked = 0;

			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): SERVICE menu end\n", key);
			#endif
			return;
		}
		else if(IS_KEYBOARD(key)) //K1-K8
		{
			if(service_seq_cnt<8)
			{
				service_seq *= 10;
				service_seq += key;
				service_seq_cnt++;
				LEDS_display_direct(~(0xff00>>(8-service_seq_cnt)),1<<(key-1));
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): SERVICE menu sequence = %llu\n", key, service_seq);
				#endif
			}
		}
		return;
	}

	if(key==KEY_SET_HELD || key==KEY_SHIFT_HELD)
	{
		if(key==KEY_SET_HELD)
		{
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): KEY_SET_HELD -> SAVE menu\n",key);
			#endif
		}
		else
		{
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): KEY_SHIFT_HELD -> LOAD menu\n", key);
			#endif
		}

		menu_function = key;
		LEDS_BLUE_OFF;
		encoder_blink = 0;

		saved_patches = find_saved_patches(bank);
		#ifdef DEBUG_OUTPUT
		printf("key_pressed(%d): find_saved_patches(): saved_patches = %x\n", key, saved_patches);
		#endif
		LEDS_display_direct(0x01<<bank,saved_patches); //orange showing bank

		#ifdef DEBUG_OUTPUT
		printf("key_pressed(%d): find_saved_patches(): last_loaded_patch bank/position = %d/%d\n", key, (last_loaded_patch-1)/8, (last_loaded_patch-1)%8+1);
		#endif
		if(last_loaded_patch>=0 && (last_loaded_patch-1)/8==bank)
		{
			LEDS_display_direct_blink(0, (last_loaded_patch-1)%8+1); //indicate most recently loaded patch
		}
		else
		{
			LEDS_display_direct_blink(0, 0);
		}

		return;
	}

	if(menu_function==KEY_SET_HELD || menu_function==KEY_SHIFT_HELD)
	{
		if(IS_KEY_EXIT(key)) //typically, both SET or SHIFT exit the menu)
		{
			menu_function = MENU_EXIT;
			last_ui_event = UI_EVENT_EXIT_MENU;
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): exit SAVE/LOAD menu\n", key);
			#endif
			LEDS_BLUE_OFF;
			LEDS_display_direct_end();
			return;
		}
		else if(IS_KEYBOARD(key)) //K1-K8
		{
			send_silence(8000);

			if(menu_function==KEY_SET_HELD)
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): store_current_patch(key)\n", key);
				#endif

				if(!arpeggiator && !sequencer)
				{
					assign_current_patch_params(); //to not lose params if updated since last load & patch triggered
				}

				//sound_enabled = SOUND_ON_WHITE_NOISE;
				//LEDS_display_direct(0,0x01<<(key-1)); //orange off, blue shows selected position
				store_current_patch(key);
				//Delay(1000);
				//LEDS_display_direct_end();
				//LEDS_BLUE_OFF;
				LEDS_display_load_save_animation(key-1, 50, 100);
				//sound_enabled = SOUND_ON;
				menu_function = MENU_EXIT;
				last_ui_event = UI_EVENT_EXIT_MENU;
				return;
			}
			if(menu_function==KEY_SHIFT_HELD)
			{
				if(saved_patches & (1<<(key-1)))
				{
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): load_current_patch(key)\n", key);
					#endif
					//sound_enabled = SOUND_OFF;
					//LEDS_display_direct(0,0x01<<(key-1)); //orange off, blue shows selected position

					//stop whatever is currently playing
					trigger_patch(PATCH_SILENCE);
					patch = PATCH_SILENCE;
					note_triggered = -1;
					wt_octave_shift = 0;
					last_note_triggered = 0;
					layer7_playhead = -1;
					layer7_last_key = -1;
					layer7_patch_modified = 0;

					load_current_patch(key);
					//Delay(1000);
					//LEDS_display_direct_end();
					//LEDS_BLUE_OFF;
					LEDS_display_load_save_animation(key-1, 50, 100);
					//sound_enabled = SOUND_ON;
					menu_function = MENU_EXIT;
					last_ui_event = UI_EVENT_EXIT_MENU;
					return;
				}
				else
				{
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): no patch stored at this position\n", key);
					#endif
				}
			}
		}
	}

	if(menu_function==MENU_PLAY && enc_function==ENC_FUNCTION_ADJUST_TEMPO)
	{
		if(IS_KEYBOARD(key))
		{
			LEDS_BLUE_BLINK(key);
			SEQUENCER_TIMING = tempo_table[key-1];
			enc_tempo_mult = 0;
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): menu_function==ADJUST_TEMPO, SEQUENCER_TIMING => %d\n", key, SEQUENCER_TIMING);
			#endif

			return; //to not trigger subsequent action immediately
		}
	}

	if(IS_KEY_ASSIGN(key) && MENU_KEY_HELD && !encoder_blink) //SHIFT + SET click
	{
		#ifdef DEBUG_OUTPUT
		printf("key_pressed(%d): IS_KEY_ASSIGN && MENU_KEY_HELD && !encoder_blink in MENU_PLAY (any layer) -> SETTINGS menu\n", key);
		#endif
		if(layer==0)
		{
			menu_function = MENU_SETTINGS; //KEY_SHIFT_SET;
			//LEDS_BLUE_GLOW2(4|LEDS_BLINK,1|LEDS_BLINK); //blink 4th and 5th LED
			indicate_sensors_base_range();
			LEDS_BLUE_GLOW2(sensors_settings_shift|LEDS_BLINK*(sensors_active%2),sensors_settings_div|LEDS_BLINK*((sensors_active/2)%2)); //blink LEDs where sensors active
		}
		else if(IS_WAVETABLE_LAYER)
		{
			menu_function = MENU_SETTINGS; //KEY_SHIFT_SET;
			LEDS_BLUE_GLOW2(4|LEDS_BLINK*(sensors_wt_layers%2),1|LEDS_BLINK*((sensors_wt_layers/2)%2)); //blink LEDs where sensors active
		}
		else if(IS_FLASH_SAMPLE_LAYER)
		{

		}
		return;
	}

	if(menu_function==MENU_SETTINGS)//KEY_SHIFT_SET) //key controls inside SETTINGS menu
	{
		if(IS_KEY_EXIT(key)) //typically, both SET or SHIFT exit the menu)
		{
			menu_function = MENU_EXIT;
			last_ui_event = UI_EVENT_EXIT_MENU;
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): exit SETTINGS menu\n", key);
			#endif
			LEDS_BLUE_OFF;
			led_indication_refresh = -1; //to start blinking seq / arp indication again
			return;
		}
		else //adjust settings with keys
		{
			if(layer==0)
			{
				int value0;

				if(key>=1 && key<=4)
				{
					value0 = sensors_settings_shift;

					if(key==4)
					{
						sensors_settings_shift = 4;
						bytebeat_song_pos_shift = 0;
					}
					else if(key==3)
					{
						sensors_settings_shift = 3;
						bytebeat_song_pos_shift = 1024;
					}
					else if(key==2)
					{
						sensors_settings_shift = 2;
						bytebeat_song_pos_shift = 2048;
					}
					else if(key==1)
					{
						sensors_settings_shift = 1;
						bytebeat_song_pos_shift = 4096;
					}

					if(value0 == sensors_settings_shift)
					{
						sensors_active ^= 0x00000001; //flip first bit
					}

				} else if(key>=5 && key<=8) {

					value0 = sensors_settings_div;

					if(key==5)
					{
						sensors_settings_div = 1;
						bytebeat_song_length_div = 1;
					}
					else if(key==6)
					{
						sensors_settings_div = 2;
						bytebeat_song_length_div = 2;
					}
					else if(key==7)
					{
						sensors_settings_div = 3;
						bytebeat_song_length_div = 4;
					}
					else if(key==8)
					{
						sensors_settings_div = 4;
						bytebeat_song_length_div = 8;
					}

					if(value0 == sensors_settings_div)
					{
						sensors_active ^= 0x00000002; //flip second bit
					}
				}
				LEDS_BLUE_GLOW2(sensors_settings_shift|LEDS_BLINK*(sensors_active%2),sensors_settings_div|LEDS_BLINK*((sensors_active/2)%2));
			}
			else if(IS_WAVETABLE_LAYER)
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): SETTINGS menu && IS_WAVETABLE_LAYER\n", key);
				#endif
				if(key==4 || key==5)
				{
					if(key==4)
					{
						sensors_wt_layers ^= 0x00000001; //flip first bit
					}
					else if(key==5)
					{
						sensors_wt_layers ^= 0x00000002; //flip second bit
					}
					LEDS_BLUE_GLOW2(4|LEDS_BLINK*(sensors_wt_layers%2),1|LEDS_BLINK*((sensors_wt_layers/2)%2)); //blink LEDs where sensors active
				}
				else if(key==2 || key==7)
				{
					if(key==2)
					{
						wt_cycle_alignment ^= 0x00000001; //flip first bit
					}
					else if(key==7)
					{
						wt_cycle_alignment ^= 0x00000002; //flip second bit
					}
					LEDS_BLUE_GLOW2(2|LEDS_BLINK*(wt_cycle_alignment%2),3|LEDS_BLINK*((wt_cycle_alignment/2)%2)); //blink LEDs where alignment active
				}

				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): SETTINGS menu && IS_WAVETABLE_LAYER: sensors_wt_layers => %02x\n", key, sensors_wt_layers);
				#endif
			}
		}
		return;
	}

	if(MENU_KEY_HELD)
	{
		//if(key==MENU_VOL || key==MENU_DEL || key==MENU_BIT || key==MENU_VAR || MENU_FRM || MENU_ARP)
		if(IS_KEYBOARD(key))
		{
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): menu_function\n",key);
			#endif
			menu_function = key;
			LEDS_BLUE_BLINK(key);
			//led_indication_refresh = -1;

			if(menu_function==MENU_LYR)
			{
				LEDS_display_direct(0,layers_active);
				LEDS_display_direct_blink(0, layer+1);
			}

			if(menu_function==MENU_VOL)
			{
				BB_SHIFT_VOLUME_0 = BB_SHIFT_VOLUME;
			}

			return; //to not trigger subsequent action immediately
		}
	}

	if(IS_KEYBOARD(key) && IS_MENU_FUNCTION) //some of the 8 keys pressed while a menu function already selected
	{
		if(menu_function==MENU_VOL)
		{
			if(layer==0)
			{
				if(BB_SHIFT_VOLUME < (key - (8-BB_VOLUME_MAX)))
				{
					BB_SHIFT_VOLUME++;
					key = BB_SHIFT_VOLUME + (8-BB_VOLUME_MAX);
				}
				else
				{
					BB_SHIFT_VOLUME = key - (8-BB_VOLUME_MAX);
				}
				//if(BB_SHIFT_VOLUME<0) { BB_SHIFT_VOLUME = 0; }
			}
			else if(IS_WAVETABLE_LAYER)
			{
				if(WAVESAMPLE_BOOST_VOLUME < (key - 4))
				{
					WAVESAMPLE_BOOST_VOLUME++;
					key = WAVESAMPLE_BOOST_VOLUME + 4;
				}
				else
				{
					WAVESAMPLE_BOOST_VOLUME = key - 4;
				}
			}
			else if(IS_FLASH_SAMPLE_LAYER)
			{
				if(FLASH_SAMPLE_BOOST_VOLUME < (key - 4))
				{
					FLASH_SAMPLE_BOOST_VOLUME++;
					key = FLASH_SAMPLE_BOOST_VOLUME + 4;
				}
				else
				{
					FLASH_SAMPLE_BOOST_VOLUME = key - 4;
				}
			}

			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): BB_SHIFT_VOLUME => %d, WAVESAMPLE_BOOST_VOLUME => %d, FLASH_SAMPLE_BOOST_VOLUME => %d\n", key, BB_SHIFT_VOLUME, WAVESAMPLE_BOOST_VOLUME, FLASH_SAMPLE_BOOST_VOLUME);
			#endif
			LEDS_BLUE_GLOW(key);
		}

		if(menu_function==MENU_DEL)
		{
			echo_dynamic_loop_current_step = key - 1;
			echo_dynamic_loop_length = echo_dynamic_loop_steps[echo_dynamic_loop_current_step];
			bytebeat_echo_on = (echo_dynamic_loop_length>0);

			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): bytebeat_echo_on => %d, echo_dynamic_loop_length => %d\n", key, bytebeat_echo_on, echo_dynamic_loop_length);
			#endif
			LEDS_BLUE_GLOW(key);
		}

		if(menu_function==MENU_BIT && !IS_WAVETABLE_LAYER)
		{
			if(layer==0)
			{
				bit1 = key;
				bit2 = 2;//key+1;
				//if(bit2>8) { bit2 = 1; }

				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d):MENU_BIT bit1 => %d, bit2 => %d\n", key, bit1, bit2);
				#endif

				LEDS_BLUE_GLOW(key);
			}
			else if(IS_FLASH_SAMPLE_LAYER)
			{
				if(layer7_last_key >= 0)
				{
					layer7_patch_bitrate[layer7_last_key] = 8-key;
					if(key==1) //skip "7" as bitrate divider
					{
						layer7_patch_bitrate[layer7_last_key] = 8;
					}
					layer7_trigger_sample(layer7_last_key);
					layer7_patch_modified = 1;

					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d):MENU_BIT layer7_patch_bitrate=%d\n", key, layer7_patch_bitrate[layer7_last_key]);
					#endif

					LEDS_BLUE_GLOW(key);
				}
			}
		}

		if(menu_function==MENU_VAR)
		{
			variable_n = (key-1)%4;
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): variable_n => %d\n", key, variable_n);
			#endif

			if(key>4)
			{
				var_p[variable_n] = 0;
				//LEDS_BLUE_GLOW2((variable_n+1)|LEDS_BLINK,variable_n+1)
				LEDS_BLUE_BLINK(variable_n+1)
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): var_p[%d] => 0\n", key, variable_n);
				#endif
			}
			else
			{
				//LEDS_BLUE_GLOW2(variable_n+1,variable_n+1)
				LEDS_BLUE_GLOW(variable_n+1)
			}
		}

		if(menu_function==MENU_FRM)
		{
			if(bytebeat_song==key-1)
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): bytebeat_song for this key already equal, resetting parameters to defaults\n", key);
				#endif
				bytebeat_song_ptr = 0;
				bytebeat_song_start = 0;
				bytebeat_song_start_effective = 0;
				bytebeat_song_length = BYTEBEAT_LENGTH_UNLIMITED;
				bytebeat_song_length_effective = BYTEBEAT_LENGTH_UNLIMITED;
				bytebeat_song_length_div = 1;
				bytebeat_song_pos_shift = 0;
			}
			else
			{
				bytebeat_song = key-1;
			}
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): bytebeat_song => %d\n", key, bytebeat_song);
			#endif
			LEDS_BLUE_GLOW(key);
		}

		if(menu_function==MENU_ARP)
		{
			if(!arpeggiator) //only change layer if it was not already running
			{
				arp_layer = layer;

				if(IS_WAVETABLE_LAYER || IS_FLASH_SAMPLE_LAYER)
				{
					arp_octave = 0;
					arp_range_from = 0;
					arp_range_to = 0;
				}
			}

			arpeggiator = key;
			arp_seq_dir = 1;
			arp_ptr = -1;
			arp_rep_cnt = 0;
			sequencer_counter = 0;

			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): arpeggiator => %d\n", key, arpeggiator);
			#endif
			LEDS_BLUE_GLOW(key);
			sequencer = 0; //can't have both at the same time
			seq_running = 0;
		}

		if(menu_function==MENU_SEQ)
		{
			sequencer = key;
			seq_layer = layer;

			if(IS_WAVETABLE_LAYER || IS_FLASH_SAMPLE_LAYER)
			{
				seq_octave = 0;
				seq_range_from = 0;
				seq_range_to = 0;
				//seq_octave_mode = 0;
			}

			//sequencer_counter = 0;
			init_sequence(sequencer-1); //sequence positions are numbered from 0
			//seq_step_ptr = 0;
			//seq_running = SEQUENCE_COMPLETE(sequencer)?1:0;
			start_stop_sequencer(sequencer);

			//divide_sequencer_steps(sequencer_steps[sequencer-1], &seq_steps_r1, &seq_steps_r2);
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): sequencer => %d, steps=%d(%dx%d), running=%d\n", key, sequencer, sequencer_steps[sequencer-1], seq_steps_r1[sequencer-1], seq_steps_r2[sequencer-1], seq_running);
			#endif

			if(seq_running)
			{
				LEDS_display_direct_end();
				LEDS_BLUE_GLOW(key);
			}
			else
			{
				indicate_sequencer_steps(seq_steps_r1[sequencer-1], seq_steps_r2[sequencer-1]);
			}
			arpeggiator = 0; //can't have both at the same time
		}

		if(menu_function==MENU_LYR)
		{
			int layer0 = layer; //need to know which layer was originally active
			layer = key-1;
			//layers_active |= (0x01 & 0x01<<(key-1);

			if(layer0==0 && layer==0 && (layers_active&0xfe)) //layer 1 key pressed while it was already active and some higher layers are active too
			{
				//printf("key_pressed(%d): layer0==0 && layer==0 && (layers_active&0xfe)\n", key);
				//if(layers_active&0x01) //layer 1 was active
				//{
					layers_active &= 0xfe; //switch layer 1 off

					//switch to some of the active higher layers, preferably a wave table layer
					if(layers_active&0x02) layer = 1;
					else if(layers_active&0x04) layer = 2;
					else if(layers_active&0x08) layer = 3;
					else if(layers_active&0x10) layer = 4;
					else if(layers_active&0x20) layer = 5;
					else if(layers_active&0x40) layer = 6;
					else if(layers_active&0x80) layer = 7;
				//}
				//else
				//{
				//	layers_active |= 0x01; //switch layer 1 on
				//}
			}
			else if(layer==0 && !(layers_active&0x01)) //layer 1 key pressed while it was not active
			{
				//printf("key_pressed(%d): layer==0 && !(layers_active&0x01)\n", key);
				layers_active |= 0x01; //switch layer 1 on
			}
			else if(IS_WAVETABLE_LAYER) //a higher layer key pressed
			{
				//if pressed while already active, check if any other layer remaining active, otherwise do not disable it
				if(layer0==layer && (layers_active & 0x81))
				{
					layers_active &= 0x81; //reset the wavetable layer bits

					//switch to some of the remaining active layers, preferably the bytebeat
					if(layers_active&0x01) layer = 0;
					else if(layers_active&0x80) layer = 7;
				}
				else
				{
					//printf("key_pressed(%d): layer>=1 && layer<=WAVETABLE_LAYERS\n", key);
					//layers_active &= 0x01; //clear the higher layer bits
					layers_active &= 0x81; //clear the wave table layer bits
					layers_active |= 0x01<<(key-1); //switch on the required bit
				}
			}
			else if(IS_FLASH_SAMPLE_LAYER) //flash sample layer key pressed
			{
				//printf("key_pressed(%d): IS_FLASH_SAMPLE_LAYER\n", key);
				if(layers_active&0x80) //while it was active
				{
					//check if any other layer remaining active, otherwise do not disable it
					if(layer0==LAYER_FLASH_SAMPLE && (layers_active & 0x7f))
					{
						layers_active &= 0x7f; //reset the highest layer bit

						//switch to some of the remaining active layers, preferably the bytebeat
						if(layers_active&0x01) layer = 0;
						else if(layers_active&0x02) layer = 1;
						else if(layers_active&0x04) layer = 2;
						else if(layers_active&0x08) layer = 3;
						else if(layers_active&0x10) layer = 4;
						else if(layers_active&0x20) layer = 5;
						else if(layers_active&0x40) layer = 6;
					}
				}
				else //it was not active before
				{
					//layers_active &= 0x81; //clear the higher layer bits
					layers_active |= 0x80; //set the highest layer bit
				}
			}

			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): layer %d => %d, layers_active => 0x%x\n", key, layer0, layer, layers_active);
			#endif
			//LEDS_BLUE_GLOW(key);
			LEDS_display_direct(0,layers_active);
			LEDS_display_direct_blink(0, layer+1);
			//led_indication_refresh = 0;

			if(IS_FLASH_SAMPLE_LAYER)
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): layer #7 (LAYER_FLASH_SAMPLE)\n", key);
				#endif
				flash_map_samples();
				FLASH_SAMPLE_FORMAT = get_sample_format();
			}
		}

		led_indication_refresh = 0; //to reset back to blinking menu LED later
		return;
	}

	//if(menu_function==MENU_VOL || menu_function==MENU_DEL || menu_function==MENU_BIT || menu_function==MENU_VAR || menu_function==MENU_FRM || menu_function==MENU_ARP)
	if(IS_MENU_FUNCTION) //menu_function>=1 && menu_function<=8
	{
		if(menu_function==MENU_ARP && IS_KEY_ASSIGN(key)) //SET turns the arpeggiator off
		{
			arpeggiator = ARP_NONE;
			menu_function = MENU_EXIT;
			last_ui_event = UI_EVENT_EXIT_MENU;
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): arpeggiator => %d, exit CONTEXT menu\n", key, arpeggiator);
			#endif
			LEDS_BLUE_OFF;
			if(arp_layer==0)
			{
				trigger_patch(PATCH_SILENCE);
			}
			return;
		}
		else if(menu_function==MENU_SEQ && IS_KEY_ASSIGN(key)) //SET turns the sequencer off, unless steps changed by encoders
		{
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): menu_function==MENU_SEQ && IS_KEY_ASSIGN, keys_or_encoders_recently = %d, led_display_direct = %d\n", key, keys_or_encoders_recently, led_display_direct);
			#endif

			if(!led_display_direct)//keys_or_encoders_recently==KEYS_RECENTLY)
			{
				sequencer = 0;
				seq_running = 0;
				menu_function = MENU_EXIT;
				last_ui_event = UI_EVENT_EXIT_MENU;
				LEDS_BLUE_OFF;
				trigger_patch(PATCH_SILENCE);
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): sequencer => %d, exit CONTEXT menu\n", key, sequencer);
				#endif
			}
			else //if(keys_or_encoders_recently==ENCODERS_RECENTLY)
			{
				/*
				LEDS_display_direct_end();
				//indicate selected sequencer again
				LEDS_BLUE_GLOW(sequencer);
				//decide if should run straight away
				start_stop_sequencer(sequencer);
				led_indication_refresh = 0; //to reset back to blinking menu LED later
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): LEDS_display_direct_end(), sequencer=%d, seq_running=%d\n", key, sequencer, seq_running);
				#endif
				*/

				//if(menu_function==MENU_SEQ && led_display_direct) //exit from the adjustment of encoder steps
				{
					start_stop_sequencer(sequencer);
					if(!seq_running) //need to program steps, reset indicator to start
					{
						led_disp[LEDS_ORANGE2] = 0;
						led_disp[LEDS_ORANGE1] = 1;
					}
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): menu_function==MENU_SEQ && led_display_direct (steps adjusted), sequencer=%d, seq_running=%d\n", key, sequencer, seq_running);
					#endif
				}

				menu_function = MENU_EXIT;
				last_ui_event = UI_EVENT_EXIT_MENU;
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): exit CONTEXT menu\n", key);
				#endif
				LEDS_display_direct_end(); //in case direct indication was active
				LEDS_BLUE_OFF;
				return;
			}

			if(!seq_running) //need to program steps, reset indicator to start
			{
				led_disp[LEDS_ORANGE2] = 0;
				led_disp[LEDS_ORANGE1] = 1;
			}
			return;
		}
		else if(IS_KEY_EXIT(key)) //typically, both SET or SHIFT exit the menu
		{
			if(menu_function==MENU_SEQ && led_display_direct) //exit from the adjustment of encoder steps
			{
				/*
				start_stop_sequencer(sequencer);
				if(!seq_running) //need to program steps, reset indicator to start
				{
					led_disp[LEDS_ORANGE2] = 0;
					led_disp[LEDS_ORANGE1] = 1;
				}
				*/

				sequencer = 0;
				seq_running = 0;
				if(seq_layer==0)
				{
					trigger_patch(PATCH_SILENCE);
				}

				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): menu_function==MENU_SEQ && led_display_direct (steps adjustment - EXIT) sequencer=%d, seq_running=%d\n", key, sequencer, seq_running);
				#endif
			}
			else if(menu_function==MENU_VOL)
			{
				if(BB_SHIFT_VOLUME_0 != BB_SHIFT_VOLUME)
				{
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): EXIT from menu_function==MENU_VOL, BB_SHIFT_VOLUME_0 != BB_SHIFT_VOLUME, storing the setting\n", key);
					#endif
					set_bb_volume(BB_SHIFT_VOLUME);
					BB_SHIFT_VOLUME_0 = BB_SHIFT_VOLUME;
				}
			}


			menu_function = MENU_EXIT;
			last_ui_event = UI_EVENT_EXIT_MENU;
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): exit CONTEXT menu\n", key);
			#endif
			LEDS_display_direct_end(); //in case direct indication was active
			LEDS_BLUE_OFF;
			return;
		}
	}

	if(menu_function==MENU_PLAY && (layer==0 || (arpeggiator && ARP_LAYER_MATCHING) || (sequencer && SEQ_LAYER_MATCHING)))
	{
		//programming the sequences while sequencer is not running: skipped beats (pause) or continuing patch
		if(sequencer && !seq_running && SEQ_LAYER_MATCHING && (IS_KEY_SHIFT(key) || IS_KEY_SET(key)))
		{
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): seq_pattern[%d][%d] position -> %s\n", key, sequencer-1, seq_step_ptr, IS_KEY_SHIFT(key)?"SEQ_PATTERN_CONTINUE":"SEQ_PATTERN_PAUSE");
			#endif
			seq_pattern[sequencer-1][seq_step_ptr] = IS_KEY_SHIFT(key)?SEQ_PATTERN_CONTINUE:SEQ_PATTERN_PAUSE;

			//skipped beat (silence) indicated by no blue LED
			if(IS_KEY_SET(key))
			{
				LEDS_BLUE_OFF;
				if(layer==0)
				{
					patch = PATCH_SILENCE;
					trigger_patch(patch);
				}
			}

			seq_filled_steps[sequencer-1]++;
			#ifdef DEBUG_OUTPUT
			printf("HERE[1]: seq_filled_steps[sequencer-1] => %d\n", seq_filled_steps[sequencer-1]);
			#endif
			if(seq_filled_steps[sequencer-1]>SEQ_STEPS_MAX)
			{
				seq_filled_steps[sequencer-1] = SEQ_STEPS_MAX;
			}
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): seq_pattern[%d][%d] => %d, seq_filled_steps[%d] => %d\n", key, sequencer-1, seq_step_ptr, seq_pattern[sequencer-1][seq_step_ptr], sequencer-1, seq_filled_steps[sequencer-1]);
			#endif

			animate_sequencer(); //progress the indication while not running yet
			seq_step_ptr++;

			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): SEQUENCE_COMPLETE? seq_filled_steps[%d] = %d, sequencer_steps[%d] = %d\n", key, sequencer-1, seq_filled_steps[sequencer-1], sequencer-1, sequencer_steps[sequencer-1]);
			#endif
			if(SEQUENCE_COMPLETE(sequencer))
			{
				seq_running = 1;
				seq_step_ptr = SEQ_STEP_START;
				sampleArpSeq = 0;
			}
			return;
		}
		else if(sequencer && SEQ_LAYER_MATCHING && IS_KEYBOARD(key)) //program new notes or update existing while sequencer is running
		{
			if(seq_step_ptr==SEQ_STEP_START)
			{
				seq_step_ptr = 0;
			}

			int update_following_beat = 0, seq_step_ptr_effective = seq_step_ptr;
			if(seq_running) //detect nearest beat (as the key might have been touched a bit too early)
			{
				if(sampleArpSeq >= SEQUENCER_TIMING/2) //too early, should update the following beat
				{
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): updating the following beat\n", key);
					#endif
					update_following_beat = 1;
					seq_step_ptr_effective++;
					if(seq_step_ptr_effective>=sequencer_steps[sequencer-1])
					{
						seq_step_ptr_effective = 0;
					}
				}
				else //should update the current beat
				{
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): updating the current beat\n", key);
					#endif
				}
			}

			//program or update the sequencer, might need to update following step instead of current one, hence using seq_step_ptr_effective
			if(seq_pattern[sequencer-1][seq_step_ptr_effective])
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): seq_pattern[%d][%d] position filled with %d\n", key, sequencer-1, seq_step_ptr_effective, seq_pattern[sequencer-1][seq_step_ptr_effective]);
				#endif
				seq_pattern[sequencer-1][seq_step_ptr_effective] = key;
			}
			else //this only applies to filling new sequence while sequencer not running, always update current step
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): seq_pattern[%d][%d] position empty\n", key, sequencer-1, seq_step_ptr);
				#endif
				seq_pattern[sequencer-1][seq_step_ptr] = key;
				seq_filled_steps[sequencer-1]++;
				#ifdef DEBUG_OUTPUT
				printf("HERE[2]: seq_filled_steps[sequencer-1] => %d\n", seq_filled_steps[sequencer-1]);
				#endif
				if(seq_filled_steps[sequencer-1]>SEQ_STEPS_MAX)
				{
					seq_filled_steps[sequencer-1] = SEQ_STEPS_MAX;
				}
				#ifdef DEBUG_OUTPUT
				if(seq_running)
				{
					printf("----------------PROBLEM PROBLEM PROBLEM PROBLEM PROBLEM PROBLEM PROBLEM PROBLEM----------------\n");
				}
				#endif
			}
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): seq_pattern[%d][%d] => %d, seq_filled_steps[%d] => %d\n", key, sequencer-1, seq_step_ptr_effective, key, sequencer-1, seq_filled_steps[sequencer-1]);
			#endif

			if(!seq_running)
			{
				animate_sequencer(); //progress the indication while not running yet
				seq_step_ptr++; //only advance the pointer while sequencer is not running yet
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): SEQUENCE_COMPLETE? seq_filled_steps[%d] = %d, sequencer_steps[%d] = %d\n", key, sequencer-1, seq_filled_steps[sequencer-1], sequencer-1, sequencer_steps[sequencer-1]);
				#endif

				if(SEQUENCE_COMPLETE(sequencer)) //check if sequencer should be started
				{
					seq_running = 1;
					seq_step_ptr = SEQ_STEP_START;
					sampleArpSeq = 0;
				}
			}

			LEDS_BLUE_GLOW(key);

			if(!update_following_beat) //do not trigger patch if following updated, as it will trigger soon on its own
			{
				if(layer==0)
				{
					patch = key-1;
					trigger_patch(patch);
				}
				else if(IS_WAVETABLE_LAYER)
				{
					note_triggered = key;
					wt_octave_shift = 0;
				}
				else if(IS_FLASH_SAMPLE_LAYER)
				{
					layer7_playhead = layer7_patch_start[key-1];
					layer7_grain_length = layer7_patch_length[key-1];
					layer7_grain_bitrate = layer7_patch_bitrate[key-1];
				}
			}
			return;
		}
		else if(sequencer && seq_running && SEQ_LAYER_MATCHING && (key==KEY_SHIFT_CLICKED || key==KEY_SET_CLICKED)) //update skipped and continued beats
		{
			if(seq_step_ptr==SEQ_STEP_START)
			{
				seq_step_ptr = 0;
			}

			int update_following_beat = 0, seq_step_ptr_effective = seq_step_ptr;
			//detect nearest beat (as the key might have been touched a bit too early)
			if(sampleArpSeq >= SEQUENCER_TIMING/2) //too early, should update the following beat
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): updating the following beat\n", key);
				#endif
				update_following_beat = 1;
				seq_step_ptr_effective++;
				if(seq_step_ptr_effective>=sequencer_steps[sequencer-1])
				{
					seq_step_ptr_effective = 0;
				}
			}
			else //should update the current beat
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): updating the current beat\n", key);
				#endif
			}

			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): seq_pattern[%d][%d] position -> %s\n", key, sequencer-1, seq_step_ptr_effective, key==KEY_SHIFT_CLICKED?"SEQ_PATTERN_CONTINUE":"SEQ_PATTERN_PAUSE");
			#endif


			if(layer==0 || key==KEY_SET_CLICKED) //SHIFT does not do anything on higher layers, only SET
			{
				seq_pattern[sequencer-1][seq_step_ptr_effective] = key==KEY_SHIFT_CLICKED?SEQ_PATTERN_CONTINUE:SEQ_PATTERN_PAUSE;
			}

			if(!update_following_beat && key==KEY_SET_CLICKED) //only trigger patch if updating the current one and it is silence
			{
				if(layer==0)
				{
					patch = PATCH_SILENCE;
					trigger_patch(patch);
				}
			}
			return;
		}
		else if(arpeggiator && IS_KEYBOARD(key) && ARP_LAYER_MATCHING)
		{
			if(arp_pattern[key-1])
			{
				if(arp_active_steps > 1)
				{
					arp_pattern[key-1] = 0;
					arp_active_steps--;
				}
				else
				{
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): cannot remove the last remaining arpeggiator step\n", key);
					#endif
				}
			}
			else
			{
				/*
				if(arpeggiator==ARP_ORDER)
				{
					for(int i=0;i<ARP_MAX_STEPS;i++)
					{
						if(arp_pattern[i]) arp_pattern[i]++;
					}
				}
				*/
				arp_pattern[key-1] = 1;
				arp_active_steps++;
			}
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): arp_pattern={%d,%d,%d,%d,%d,%d,%d,%d}, arp_active_steps=%d\n", key, arp_pattern[0], arp_pattern[1], arp_pattern[2], arp_pattern[3], arp_pattern[4], arp_pattern[5], arp_pattern[6], arp_pattern[7], arp_active_steps);
			#endif
			return;
		}
		else if(IS_KEYBOARD(key) && !encoder_blink && layer==0) //direct play
		{
			LEDS_BLUE_GLOW(key);

			//load parameters
			patch = key-1;

			//printf("K1-K8 pressed, key=%d\n", key);
			//{
				trigger_patch(patch);
			//}
			return;
		}
	}

	if(menu_function==MENU_PLAY && IS_WAVETABLE_LAYER)
	{
		if(IS_KEYBOARD(key))
		{
			note_triggered = key;
			wt_octave_shift = 0;

			if(encoder_blink && last_note_triggered>0) //SET clicked previously and some note already played
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): assign tuning to mini piano key, %d => %d, layer = %d\n", key, last_note_triggered, note_triggered, layer);
				#endif
				mini_piano_tuning[note_triggered-1] = mini_piano_tuning[last_note_triggered-1];
				mini_piano_waves[note_triggered-1] = mini_piano_waves[last_note_triggered-1];
				rnd_envelope_div[note_triggered-1] = rnd_envelope_div[last_note_triggered-1];
			}

			encoder_blink = 0;
			last_note_triggered = key;
			LEDS_BLUE_GLOW(key);
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): note_updated => %d (layer %d)\n", key, note_triggered, layer);
			#endif
			return;
		}
	}

	if(menu_function==MENU_PLAY && ((layer==0 && !arpeggiator && !sequencer) || (arpeggiator && arp_layer!=layer) || (sequencer && seq_layer!=layer) || IS_WAVETABLE_LAYER || IS_FLASH_SAMPLE_LAYER))
	{
		#ifdef DEBUG_OUTPUT
		printf("key_pressed(%d): menu_function==MENU_PLAY && ((layer==0 && !arpeggiator && !sequencer) || IS_WAVETABLE_LAYER || IS_FLASH_SAMPLE_LAYER)\n", key);
		#endif

		if((IS_KEY_ASSIGN(key) || (IS_KEYBOARD(key) && !ASSIGN_KEY_HELD)) && encoder_blink) //SET or K1-K8 pressed (while SET not held anymore)
		{
			if(IS_KEY_ASSIGN(key))
			{
				if(layer==0)
				{
					if(bb_last_note_triggered>=0)
					{
						#ifdef DEBUG_OUTPUT
						printf("key_pressed(%d): IS_KEY_ASSIGN in MENU_PLAY && encoder_blink -> assign new params to current patch\n", key);
						#endif
						LEDS_BLUE_GLOW(patch+1);
					}
				}
				else //if(IS_WAVETABLE_LAYER) //if mini piano or flash sample layers
				{
					LEDS_BLUE_OFF;
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): mini piano or flash sample layers[0]\n", key);
					#endif
				}

				last_ui_event=UI_EVENT_EXIT_MENU; //prevent counting SET click again
			}
			else
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): IS_KEYBOARD in MENU_PLAY && encoder_blink -> assign new params to a patch selected by key\n", key);
				#endif
				LEDS_BLUE_GLOW(key);
				if(layer==0)
				{
					patch = key-1;
				}
				else if(IS_WAVETABLE_LAYER) //if mini piano layers
				{
					note_triggered = key-1;
					wt_octave_shift = 0;
					#ifdef DEBUG_OUTPUT
					//printf("key_pressed(%d): layer = %d, note_triggered => %d\n", key, layer, note_triggered);
					printf("key_pressed(%d): mini piano layers[1]\n", key);
					#endif
				}
				else if(IS_FLASH_SAMPLE_LAYER && layer7_last_key>=0) //if flash sample layer and some note already played
				{
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): assign sample params to key, %d => %d, layer = %d\n", key, layer7_last_key, key-1, layer);
					#endif

					layer7_patch_start[key-1] = layer7_patch_start[layer7_last_key];
					layer7_patch_length[key-1] = layer7_patch_length[layer7_last_key];
					layer7_patch_bitrate[key-1] = layer7_patch_bitrate[layer7_last_key];

					layer7_trigger_sample(key-1);
					LEDS_BLUE_GLOW(key);

					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): note_updated => %d (layer %d)\n", key, layer7_last_key, layer);
					#endif
				}
			}
			encoder_blink = 0;

			//assign current parameters
			if(layer==0)
			{
				if(bb_last_note_triggered>=0)
				{
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): layer==0, assign_current_patch_params()\n", key);
					#endif
					assign_current_patch_params();
				}
			}
			else //if(IS_WAVETABLE_LAYER) //if mini piano layers
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): mini piano or flash sample layers[2]\n", key);
				#endif
			}
			return;
		}
		//else if(IS_KEY_ASSIGN(key) && !MENU_KEY_HELD) //activate the update-patch using SET while nothing runs
		else if(key==KEY_SET_CLICKED && !MENU_KEY_HELD) //activate the update-patch using SET while nothing runs
		{
			#ifdef DEBUG_OUTPUT
			//printf("key_pressed(%d): IS_KEY_ASSIGN && !MENU_KEY_HELD in MENU_PLAY\n", key);
			printf("key_pressed(%d): KEY_SET_CLICKED && !MENU_KEY_HELD in MENU_PLAY\n", key);
			#endif
			if(!encoder_blink)
			{
				if(layer==0)
				{
					LEDS_BLUE_BLINK(patch+1);
					encoder_blink = 1;
				}
				else if(IS_WAVETABLE_LAYER && last_note_triggered>0) //if mini piano layers
				{
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): mini piano layers[3], LEDS_BLUE_BLINK(%d)\n", key, last_note_triggered);
					#endif
					LEDS_BLUE_BLINK(last_note_triggered);
					encoder_blink = 1;
				}
				else if(IS_FLASH_SAMPLE_LAYER && layer7_last_key>=0) //flash sample layer
				{
					#ifdef DEBUG_OUTPUT
					printf("key_pressed(%d): flash sample layer, LEDS_BLUE_BLINK(%d)\n", key, layer7_last_key+1);
					#endif
					LEDS_BLUE_BLINK(layer7_last_key+1);
					encoder_blink = 1;
				}
			}
			return;
		}
		else if(IS_KEY_MENU(key) && encoder_blink) //cancel
		{
			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): IS_KEY_MENU && encoder_blink in MENU_PLAY -> no assign, exit\n", key);
			#endif
			if(layer==0)
			{
				LEDS_BLUE_GLOW(patch+1);
			}
			else //if mini piano or sample layers
			{
				#ifdef DEBUG_OUTPUT
				printf("key_pressed(%d): mini piano or sample layers[4]\n", key);
				#endif
			}
			encoder_blink = 0;
			return;
		}
		/*
		//this is here for possible future use
		else if(IS_KEYBOARD(key) && ASSIGN_KEY_HELD) //SET + K1-K8 combination (while SET held)
		{
			printf("key_pressed(%d): IS_KEYBOARD & ASSIGN_KEY_HELD (SET + K1-K8)\n", key);
			LEDS_BLUE_GLOW(key);
			encoder_blink = 0;
			set_held = -1; //cancel waiting for SET_HELD_TIMEOUT
			return;
		}
		*/
	}

	if(menu_function==MENU_PLAY && IS_FLASH_SAMPLE_LAYER)
	{
		if(IS_KEYBOARD(key))
		{
			//layer7_playhead = SAMPLES_LENGTH/8*(key-1);
			//layer7_grain_length = SAMPLES_LENGTH/8;

			layer7_trigger_sample(key-1);
			LEDS_BLUE_GLOW(key);

			#ifdef DEBUG_OUTPUT
			printf("key_pressed(%d): layer7_playhead=%d, layer7_grain_length=%d, layer7_grain_bitrate=%d\n", key, layer7_playhead, layer7_grain_length, layer7_grain_bitrate);
			#endif
			return;
		}
	}

	/*
	if(led_disp[LEDS_ORANGE] > LEDS_BLINK) //shift already pressed
	{
		sequencer_blink = 0;

		if(key==TOUCH_PAD_LEFT)
		{
			led_disp[LEDS_ORANGE] = 0;
			led_disp[LEDS_ORANGE1] = 0;
			led_disp[LEDS_ORANGE2] = 0;
			menu_function = MENU_PLAY;
		}
		else
		{
			led_disp[LEDS_ORANGE] = key;

			if(led_disp[LEDS_ORANGE]<=4)
			{
				led_disp[LEDS_ORANGE1] = key;
				led_disp[LEDS_ORANGE2] = 0;
			}
			else
			{
				led_disp[LEDS_ORANGE1] = 0;
				led_disp[LEDS_ORANGE2] = key-4;
			}

			menu_function = key;
		}
	}
	else
	{
	*/
	//}
	//printf("key_pressed(%d): led_disp[LEDS_ORANGE]=%d, menu_function=%d\n",key,led_disp[LEDS_ORANGE],menu_function);
}

void scan_keypad()
{
	if(led_indication_refresh >= 0)
	{
		led_indication_refresh++;
	}

	if(menu_function==MENU_PLAY)
	{
		if(led_indication_refresh == LED_INDICATION_REFRESH_TIMEOUT)
		{
			#ifdef DEBUG_OUTPUT
			printf("MENU_PLAY && led_indication_refresh == LED_INDICATION_REFRESH_TIMEOUT\n");
			#endif
			//LEDS_BLUE_BLINK(menu_function);
			led_indication_refresh = -1;
			//LEDS_display_direct_end();
		}
	}

	if(menu_function && menu_function!=MENU_SETTINGS)
	{
		if(led_indication_refresh == LED_INDICATION_REFRESH_TIMEOUT)
		{
			#ifdef DEBUG_OUTPUT
			printf("menu_function && led_indication_refresh == LED_INDICATION_REFRESH_TIMEOUT\n");
			#endif
			LEDS_BLUE_BLINK(menu_function);
			led_indication_refresh = -1;
			//LEDS_display_direct_end();
		}
	}
	else
	{
		if(touchpad_event_SET_HELD)
		{
			touchpad_event_SET_HELD = 0;
			key_pressed(KEY_SET_HELD);
			keys_or_encoders_recently = KEYS_RECENTLY;
		}
		if(touchpad_event_SHIFT_HELD)
		{
			touchpad_event_SHIFT_HELD = 0;
			key_pressed(KEY_SHIFT_HELD);
			keys_or_encoders_recently = KEYS_RECENTLY;
		}
		if(touchpad_event_BOTH_HELD)
		{
			touchpad_event_BOTH_HELD = 0;
			key_pressed(KEY_BOTH_HELD);
			keys_or_encoders_recently = KEYS_RECENTLY;
		}
	}

	if(touchpad_event[TOUCH_PAD_RIGHT]==1) //shift
	{
		touchpad_event[TOUCH_PAD_RIGHT]=0;
		//shift_pressed();
		encoder_changed = 0;
		key_pressed(KEY_SHIFT); //9
		keys_or_encoders_recently = KEYS_RECENTLY;
	}
	if(touchpad_event[TOUCH_PAD_LEFT]==1) //set
	{
		touchpad_event[TOUCH_PAD_LEFT]=0;
		encoder_changed = 0;
		key_pressed(KEY_SET); //0
		keys_or_encoders_recently = KEYS_RECENTLY;
	}

	if(touchpad_event[TOUCH_PAD_K1]==1) { key_pressed(1); touchpad_event[TOUCH_PAD_K1]=0; keys_or_encoders_recently = KEYS_RECENTLY; }
	if(touchpad_event[TOUCH_PAD_K2]==1) { key_pressed(2); touchpad_event[TOUCH_PAD_K2]=0; keys_or_encoders_recently = KEYS_RECENTLY; }
	if(touchpad_event[TOUCH_PAD_K3]==1) { key_pressed(3); touchpad_event[TOUCH_PAD_K3]=0; keys_or_encoders_recently = KEYS_RECENTLY; }
	if(touchpad_event[TOUCH_PAD_K4]==1) { key_pressed(4); touchpad_event[TOUCH_PAD_K4]=0; keys_or_encoders_recently = KEYS_RECENTLY; }
	if(touchpad_event[TOUCH_PAD_K5]==1) { key_pressed(5); touchpad_event[TOUCH_PAD_K5]=0; keys_or_encoders_recently = KEYS_RECENTLY; }
	if(touchpad_event[TOUCH_PAD_K6]==1) { key_pressed(6); touchpad_event[TOUCH_PAD_K6]=0; keys_or_encoders_recently = KEYS_RECENTLY; }
	if(touchpad_event[TOUCH_PAD_K7]==1) { key_pressed(7); touchpad_event[TOUCH_PAD_K7]=0; keys_or_encoders_recently = KEYS_RECENTLY; }
	if(touchpad_event[TOUCH_PAD_K8]==1) { key_pressed(8); touchpad_event[TOUCH_PAD_K8]=0; keys_or_encoders_recently = KEYS_RECENTLY; }

	if(shift_clicked) { key_pressed(KEY_SHIFT_CLICKED); shift_clicked=0; }
	if(set_clicked) { key_pressed(KEY_SET_CLICKED); set_clicked=0; }
}
