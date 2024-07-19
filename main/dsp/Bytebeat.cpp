/*
 * Bytebeat.cpp
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

#include <Bytebeat.h>
#include <SineWaves.h>
#include <hw/signals.h>
#include <hw/init.h>
#include <hw/gpio.h>
#include <hw/keys.h>
#include <hw/encoders.h>
#include <hw/settings.h>
#include <string.h>

//#define DEBUG_SAMPLE_LEVELS
//#define DEBUG_OUTPUT

int8_t bytebeat_song, patch;
int bytebeat_song_ptr, bytebeat_song_start=0, bytebeat_song_start_effective=0, bytebeat_song_length=BYTEBEAT_LENGTH_UNLIMITED, bytebeat_song_length_effective=BYTEBEAT_LENGTH_UNLIMITED;
uint16_t bytebeat_song_length_div = 1, bytebeat_song_pos_shift = 0;
int bb_ptr_continuous = 0, bb_last_note_triggered = -1;

/*
uint8_t bytebeat_speed_div = 0;

uint8_t bytebeat_speed = 2;
#define BYTEBEAT_SPEED_MAX 1
#define BYTEBEAT_SPEED_MIN 128
*/

#define STEREO_MIXING_STEPS 4
uint8_t stereo_mixing_step = 1; //default mixing 60:40
//uint8_t stereo_mixing_step = 2; //wider stereo
uint8_t bytebeat_echo_on = 0;

#define BYTEBEAT_ECHO_MIXING_FACTOR 0.5;//21f

uint8_t BB_SHIFT_VOLUME = BB_VOLUME_DEFAULT; //good values are 4-5
uint8_t BB_SHIFT_VOLUME_0;

int32_t SEQUENCER_TIMING = SEQUENCER_TIMING_DEFAULT;
int32_t sequencer_counter = 0;
uint8_t sequencer_blink = 0; //possibly not used
uint8_t encoder_blink = 0;
const uint32_t tempo_table[8] = {
		//1024*16, //32 beats = 16:35 sec, ~120BPM / 128 beats = 65:44
		//8000-6, //1/4 sec off at 80 sec
		//16000-12, //ok -> 1/4 sec faster over 4 min
		//32000-24, //ok -> 1/4 sec faster over 4 min

		I2S_AUDIOFREQ, //32000, //60 BPM
		I2S_AUDIOFREQ * 60 / 80, //80 BPM
		I2S_AUDIOFREQ * 60 / 90, //90 BPM
		I2S_AUDIOFREQ * 60 / 100, //100 BPM
		I2S_AUDIOFREQ / 2, //16000, //120 BPM - seems perfect still at 4:40 min
		I2S_AUDIOFREQ * 60 / 130, //130 BPM
		I2S_AUDIOFREQ * 60 / 140, //140 BPM
		I2S_AUDIOFREQ * 60 / 150, //150 BPM
	};

int patch_song_start[NUM_PATCHES] =		{0,0,0,0,0,0,0,0},
	patch_song_length[NUM_PATCHES] =	{-1,-1,-1,-1,-1,-1,-1,-1}, //BYTEBEAT_LENGTH_UNLIMITED
	patch_var1[NUM_PATCHES] =			{0,0,0,0,0,0,0,0}, //variable #1
	patch_var2[NUM_PATCHES] =			{0,0,0,0,0,0,0,0}, //variable #2
	patch_var3[NUM_PATCHES] =			{0,0,0,0,0,0,0,0}, //variable #3
	patch_var4[NUM_PATCHES] =			{0,0,0,0,0,0,0,0}, //variable #4
	patch_bit1[NUM_PATCHES] =			{4,4,4,3,4,2,4,4}, //bitspeed #1
	patch_bit2[NUM_PATCHES] =			{2,2,2,2,2,2,2,2}; //bitspeed #2
int8_t patch_song[NUM_PATCHES] =		{0,1,2,3,4,5,6,7};

uint8_t bit1,bit2;

unsigned char var_p[4] = {0,0,0,0};
int variable_n = 0, arpeggiator = 0, arp_pattern[ARP_MAX_STEPS+1] = {1,2,3,4,5,6,7,8,-1}, arp_ptr = -1, arp_active_steps, arp_rep_cnt = 0;
int arp_seq_dir = 1, arp_octave = 0, arp_repeat = 0, arp_range_from = 0, arp_range_to = 0, arp_octave_effective = 0;
int seq_octave = 0, seq_octave_mode = 0, seq_range_from = 0, seq_range_to = 0, seq_octave_effective = 0;
uint8_t arp_layer = 0, seq_layer = 0;

int8_t sequencer = 0, sequencer_steps[NUM_SEQUENCES] = {SEQ_STEPS_DEFAULT,SEQ_STEPS_DEFAULT,SEQ_STEPS_DEFAULT,SEQ_STEPS_DEFAULT,SEQ_STEPS_DEFAULT,SEQ_STEPS_DEFAULT,SEQ_STEPS_DEFAULT,SEQ_STEPS_DEFAULT};
int8_t seq_steps_r1[NUM_SEQUENCES] = {SEQ_STEPS_D_R1,SEQ_STEPS_D_R1,SEQ_STEPS_D_R1,SEQ_STEPS_D_R1,SEQ_STEPS_D_R1,SEQ_STEPS_D_R1,SEQ_STEPS_D_R1,SEQ_STEPS_D_R1};
int8_t seq_steps_r2[NUM_SEQUENCES] = {SEQ_STEPS_D_R2,SEQ_STEPS_D_R2,SEQ_STEPS_D_R2,SEQ_STEPS_D_R2,SEQ_STEPS_D_R2,SEQ_STEPS_D_R2,SEQ_STEPS_D_R2,SEQ_STEPS_D_R2};
int8_t* seq_pattern[NUM_SEQUENCES] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
int8_t seq_filled_steps[NUM_SEQUENCES] = {0,0,0,0,0,0,0,0};
int seq_step_ptr = SEQ_STEP_START, seq_running = 0, s_step;

int layer = 0, bank = 0, last_loaded_patch = -1, sound_stopped = 0;
int layer7_playhead = -1, layer7_grain_length = -1, layer7_grain_bitrate = 1, layer7_last_key = -1;
int layer7_patch_modified = 0, layer7_samples_delete = 0;

int layer7_patch_start[NUM_PATCHES] = {0, 69020, 81045, 117043, 158759, 255487, 290432, 418381};
int layer7_patch_length[NUM_PATCHES] = {70988, 13593, 37118, 45271, 89835, 30473, 127300, 92774};
int layer7_patch_bitrate[NUM_PATCHES] = {1,1,1,1,1,1,1,1};

uint8_t layers_active = 0x01;

int8_t FLASH_SAMPLE_BOOST_VOLUME = FLASH_SAMPLE_BOOST_VOLUME_DEFAULT;
int8_t FLASH_SAMPLE_FORMAT; //1=signed, 2=unsigned

int arp_step(int check_repeat)
{
	if(check_repeat && arp_repeat>0)
	{
		if(arp_repeat==8)
		{
			//to get 0,1,3,7 at output (which will always result in higher octave exactly, no fifts or thirds)
			arp_rep_cnt = rand() % 3;
			if(arp_rep_cnt==3) arp_rep_cnt = 7;
			if(arp_rep_cnt==2) arp_rep_cnt = 3;
		}
		else
		{
			if(arp_rep_cnt<arp_repeat)
			{
				arp_rep_cnt++;
				return 1;
			}
			arp_rep_cnt = 0;
		}
	}

	if(arpeggiator==ARP_UP)// || arpeggiator==ARP_UP2)
	{
		arp_ptr++;
		while(arp_ptr < ARP_MAX_STEPS && arp_pattern[arp_ptr]==0) { arp_ptr++; }
		if(arp_ptr >= ARP_MAX_STEPS)
		{
			arp_ptr = 0;
			while(arp_pattern[arp_ptr]==0) { arp_ptr++; }

			arp_octave_effective++;
			if(arp_octave_effective>arp_range_to)
			{
				arp_octave_effective = arp_range_from;
			}
		}
	}
	else if(arpeggiator==ARP_DOWN)// || arpeggiator==ARP_DOWN2)
	{
		if(arp_ptr==-1) //if right after reset, need to set to the upper note
		{
			arp_ptr = ARP_MAX_STEPS;
		}
		arp_ptr--;
		while(arp_ptr >= 0 && arp_pattern[arp_ptr]==0) { arp_ptr--; }
		if(arp_ptr < 0)
		{
			arp_ptr = ARP_MAX_STEPS - 1;
			while(arp_pattern[arp_ptr]==0) { arp_ptr--; }

			arp_octave_effective--;
			if(arp_octave_effective<arp_range_from)
			{
				arp_octave_effective = arp_range_to;
			}
		}
	}
	else if(arpeggiator==ARP_UPDN || arpeggiator==ARP_UPDN_L)
	{
		arp_ptr+=arp_seq_dir;
		//printf("arp_step(): p0[%d]\n",arp_ptr);

		while(arp_ptr>=0 && arp_ptr<ARP_MAX_STEPS && arp_pattern[arp_ptr]==0) { arp_ptr+=arp_seq_dir; }
		//printf("arp_step(): p1[%d]\n",arp_ptr);

		//at the low end of the octave range flip the direction
		if(arp_seq_dir==-1 && arp_ptr==-1 && arp_octave_effective==arp_range_from)
		{
			arp_seq_dir = 1;
			arp_ptr = 0;
			//printf("arp_step(): dp+1[%d,%d]\n",arp_dir,arp_ptr);
			while(arp_pattern[arp_ptr]==0) { arp_ptr++; }
			//printf("arp_step(): dp+2[%d,%d]\n",arp_dir,arp_ptr);
			if(arp_active_steps>1 && arpeggiator==ARP_UPDN) arp_step(0); //skip through one of the end notes
			return 1;
		}

		//at the high end of the octave range flip the direction
		else if(arp_seq_dir==1 && arp_ptr >= ARP_MAX_STEPS && arp_octave_effective==arp_range_to)
		{
			arp_seq_dir = -1;
			arp_ptr = ARP_MAX_STEPS-1;
			//printf("arp_step(): dp-1[%d,%d]\n",arp_dir,arp_ptr);
			while(arp_pattern[arp_ptr]==0) { arp_ptr--; }
			//printf("arp_step(): dp-2[%d,%d]\n",arp_dir,arp_ptr);
			if(arp_active_steps>1 && arpeggiator==ARP_UPDN) arp_step(0); //skip through one of the end notes
			return 1;
		}

		//not at the low end of the octave range
		else if(arp_seq_dir==-1 && arp_ptr==-1/* && arp_octave_effective==arp_range_from*/)
		{
			//arp_dir = 1;
			arp_octave_effective--;
			arp_ptr = ARP_MAX_STEPS-1;
			//printf("arp_step(): dp+1[%d,%d]\n",arp_dir,arp_ptr);
			while(arp_pattern[arp_ptr]==0) { arp_ptr--; }
			//printf("arp_step(): dp+2[%d,%d]\n",arp_dir,arp_ptr);
			//if(arp_active_steps>1 && arpeggiator==ARP_UPDN)arp_step(); //skip through one of the end notes
			return 1;
		}

		//not at the high end of the octave range
		else if(arp_seq_dir==1 && arp_ptr >= ARP_MAX_STEPS /*&& arp_octave_effective==arp_range_to*/)
		{
			//arp_dir = -1;
			arp_octave_effective++;
			arp_ptr = 0;
			//printf("arp_step(): dp-1[%d,%d]\n",arp_dir,arp_ptr);
			while(arp_pattern[arp_ptr]==0) { arp_ptr++; }
			//printf("arp_step(): dp-2[%d,%d]\n",arp_dir,arp_ptr);
			//if(arp_active_steps>1 && arpeggiator==ARP_UPDN)arp_step(); //skip through one of the end notes
			return 1;
		}
		#ifdef DEBUG_OUTPUT
		printf("arp_step(): ARP_UPDN/ARP_UPDN_R p=%d,o=%d\n", arp_ptr, arp_octave_effective);
		#endif
	}
	else if(arpeggiator==ARP_RANDOM || arpeggiator==ARP_RND_L)
	{
		if(arp_range_to==arp_range_from)
		{
			arp_octave_effective = arp_range_from;
		}
		else
		{
			arp_octave_effective = arp_range_from + rand() % (arp_range_to - arp_range_from + 1);
		}

		int arp_ptr0 = arp_ptr;//, rnd_retries = 0;
		arp_ptr = rand() % 8;
		while(arp_pattern[arp_ptr]==0) { arp_ptr = rand() % 8; /*rnd_retries++; arp_ptr++;*/ }
		//printf("arp_step(): ARP_RANDOM/ARP_RND_L rnd_retries=%d\n", rnd_retries);
		/*
		if(arp_ptr >= ARP_MAX_STEPS)
		{
			arp_ptr = 0;
			while(arp_pattern[arp_ptr]==0) { arp_ptr++; }
		}
		*/
		if(arpeggiator==ARP_RND_L)
		{
			//printf("arp_step(): arp0=%d,arp=%d\n",arp_ptr0,arp_ptr);
			return (arp_ptr==arp_ptr0)?0:1;
		}
	}
	else if(arpeggiator==ARP_STEP || arpeggiator==ARP_STEP_L)
	{
		if(arp_range_to==arp_range_from)
		{
			arp_octave_effective = arp_range_from;
		}
		else
		{
			arp_octave_effective = arp_range_from + rand() % (arp_range_to - arp_range_from + 1);
		}

		int arp_ptr0 = arp_ptr;
		arp_seq_dir = rand() % 3 - 1; //from -1 to 1
		#ifdef DEBUG_OUTPUT
		printf("arp_step(): arp_dir=%d\n", arp_seq_dir);
		#endif

		if(arp_seq_dir==1)
		{
			arp_ptr++;
			while(arp_ptr < ARP_MAX_STEPS && arp_pattern[arp_ptr]==0) { arp_ptr++; }
			if(arp_ptr >= ARP_MAX_STEPS)
			{
				arp_ptr = 0;
				while(arp_pattern[arp_ptr]==0) { arp_ptr++; }
			}
		}
		else if(arp_seq_dir==-1)
		{
			arp_ptr--;
			if(arp_ptr<0) { arp_ptr = ARP_MAX_STEPS - 1; }
			while(arp_ptr>=0 && arp_pattern[arp_ptr]==0)
			{
				arp_ptr--;
				if(arp_ptr<0) { arp_ptr = ARP_MAX_STEPS - 1; }
			}
		}

		if(arpeggiator==ARP_STEP_L)
		{
			#ifdef DEBUG_OUTPUT
			printf("arp_step(): arp0=%d,arp=%d\n",arp_ptr0,arp_ptr);
			#endif
			return (arp_ptr==arp_ptr0)?0:1;
		}
	}

	//printf("arp_step(): arp_ptr => %d\n", arp_ptr);
	return 1;
}

int seq_step()
{
	if(!seq_running)
	{
		return 0;
	}

	seq_step_ptr++;
	if(seq_step_ptr>=sequencer_steps[sequencer-1])
	{
		seq_step_ptr = 0;

		if(seq_octave_mode==SEQ_OCTAVE_MODE_UP)
		{
			seq_octave_effective++;
			if(seq_octave_effective>seq_range_to)
			{
				seq_octave_effective = seq_range_from;
			}
		}
		else if(seq_octave_mode==SEQ_OCTAVE_MODE_DOWN)
		{
			seq_octave_effective--;
			if(seq_octave_effective<seq_range_from)
			{
				seq_octave_effective = seq_range_to;
			}
		}
		else if(seq_octave_mode==SEQ_OCTAVE_MODE_UPDOWN)
		{
			if(arp_seq_dir==1)
			{
				if(seq_octave_effective<seq_range_to)
				{
					seq_octave_effective++;
				}
				else if(seq_range_from!=seq_range_to)
				{
					arp_seq_dir = -1;
					seq_octave_effective--;
				}
			}
			else
			{
				if(seq_octave_effective>seq_range_from)
				{
					seq_octave_effective--;
				}
				else if(seq_range_from!=seq_range_to)
				{
					arp_seq_dir = 1;
					seq_octave_effective++;
				}
			}
		}
	}
	if(seq_pattern[sequencer-1][seq_step_ptr]==SEQ_PATTERN_CONTINUE)
	{
		return 0; //no change, continue the existing patch
	}

	if(seq_octave_mode==SEQ_OCTAVE_MODE_RANDOM)
	{
		if(seq_range_to==seq_range_from)
		{
			seq_octave_effective = seq_range_from;
			//printf("seq_octave_effective[1] => %d\n", seq_octave_effective);
		}
		else
		{
			//printf("seq_octave_effective[2] => %d (seq_range_to=%d - seq_range_from=%d + 1 = %d)\n", seq_octave_effective, seq_range_to, seq_range_from, (seq_range_to - seq_range_from + 1));
			seq_octave_effective = seq_range_from + rand() % (seq_range_to - seq_range_from + 1);
			//printf("seq_octave_effective[3] => %d\n", seq_octave_effective);
		}
	}

	return seq_pattern[sequencer-1][seq_step_ptr];
}

IRAM_ATTR void bytebeat_engine()
{
	//printf("bytebeat_engine()\n");
	//#define BYTEBEAT_MCLK

	#ifdef BOARD_WHALE
	int bytebeat_echo_cycle = 4;
	#endif

	#ifdef BYTEBEAT_MCLK
	start_MCLK();
	#endif
	//codec_init(); //i2c init

	bytebeat_init();
	#ifdef BOARD_WHALE
	RGB_LED_B_ON;
	#endif

	/*
	sample32 = 0;
	for(unsigned long int i=0;i<I2S_AUDIOFREQ;i++)
	{
		i2s_write(I2S_NUM, (char *)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
	}
    */

	printf("bytebeat_engine(): loop starting\n");

	while(1)//!event_next_channel)
	{
		//if(sound_enabled==SOUND_ON)
		//{
			sample32 = bytebeat_next_sample();
		/*}
		else if(sound_enabled==SOUND_ON_WHITE_NOSE)
		{
			fill_with_random_value((char*)&sample32);
			printf("r\n");
		}
		else
		{
			sample32 = SOUND_OFF;
			printf("0\n");
		}*/

		i2s_write(I2S_NUM, (char *)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);

		process_encoders();

		if(!sound_stopped)
		{
			//if(sampleCounter % (SEQUENCER_TIMING) == 0)
			if(sampleArpSeq >= SEQUENCER_TIMING)
			{
				sampleArpSeq = 0;

				if(arpeggiator)
				{
					//printf("[a]\n");
					if(arp_step(1))
					{
						if(arp_layer==0)
						{
							//trigger_key(arp_pattern[arp_ptr]);
							patch = arp_ptr;
							//printf("trigger_patch_octave(patch=%d, arp_octave_effective=%d);\n", patch, arp_octave_effective);
							trigger_patch_octave(patch, arp_octave_effective + arp_rep_cnt);
						}
						else if(arp_layer>=1 && arp_layer<=WAVETABLE_LAYERS && arp_ptr >= 0)
						{
							note_triggered = arp_ptr+1;
							wt_octave_shift = arp_octave_effective + arp_rep_cnt;
						}
						else if(arp_layer==LAYER_FLASH_SAMPLE)
						{
							layer7_trigger_sample(arp_ptr);
						}

						if(!menu_function && led_indication_refresh==-1 && !encoder_blink)
						{
							//LEDS_BLUE_GLOW(arp_pattern[arp_ptr]);
							LEDS_BLUE_GLOW(arp_ptr+1);
						}
					}
				}

				if(sequencer)
				{
					//printf("[s]\n");
					s_step = seq_step();
					if(s_step)
					{
						if(s_step==SEQ_PATTERN_PAUSE) //silence
						{
							if(seq_layer==0)
							{
								//printf("trigger_patch(PATCH_SILENCE);\n");
								trigger_patch(PATCH_SILENCE);
								patch = PATCH_SILENCE;
							}
						}
						else
						{
							if(seq_layer==0)
							{
								patch = s_step-1; //patches are numbered from 0
								//printf("trigger_patch_octave(patch=%d, seq_octave_effective=%d);\n", patch, seq_octave_effective);
								trigger_patch_octave(patch, seq_octave_effective);
							}
							else if(seq_layer>=1 && seq_layer<=WAVETABLE_LAYERS)
							{
								note_triggered = s_step;
								wt_octave_shift = seq_octave_effective;
							}
							else if(seq_layer==LAYER_FLASH_SAMPLE)
							{
								layer7_trigger_sample(s_step-1);
							}
						}
						if(!menu_function && led_indication_refresh==-1 && !encoder_blink)
						{
							LEDS_BLUE_GLOW(s_step);
						}
					}
				}

				if(menu_function==MENU_PLAY && (arpeggiator || (sequencer && seq_running))) //no menu, blink the sequencer
				{
					animate_sequencer();
				}
				else if(menu_function==MENU_ARP && arpeggiator)
				{
					sequencer_counter++;
				}
				else if(menu_function==MENU_SEQ && seq_running)
				{
					sequencer_counter++;
				}
			}

			sampleCounter++;
			sampleArpSeq++;
		}

		/*
		if(sampleCounter % 20000 == 0)
		{
			printf("%x\n",sample32);
		}
		*/

		/*
		ui_command = 0;

		#define BYTEBEAT_UI_CMD_NEXT_SONG		1
		#define BYTEBEAT_UI_CMD_STEREO_PANNING	2
		#define BYTEBEAT_UI_CMD_SPEED_INCREASE	3
		#define BYTEBEAT_UI_CMD_SPEED_DECREASE	4

		//map UI commands
		#ifdef BOARD_WHALE
		if(short_press_volume_plus) { ui_command = BYTEBEAT_UI_CMD_NEXT_SONG; short_press_volume_plus = 0; }
		if(short_press_volume_minus) { ui_command = BYTEBEAT_UI_CMD_STEREO_PANNING; short_press_volume_minus = 0; }
		if(short_press_sequence==2) { ui_command = BYTEBEAT_UI_CMD_SPEED_INCREASE; short_press_sequence = 0; }
		if(short_press_sequence==-2) { ui_command = BYTEBEAT_UI_CMD_SPEED_DECREASE; short_press_sequence = 0; }
		#endif

		#ifdef BOARD_GECHO
		if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = BYTEBEAT_UI_CMD_NEXT_SONG; btn_event_ext = 0; }
		if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = BYTEBEAT_UI_CMD_STEREO_PANNING; btn_event_ext = 0; }
		if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_1) { ui_command = BYTEBEAT_UI_CMD_SPEED_DECREASE; btn_event_ext = 0; }
		if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_2) { ui_command = BYTEBEAT_UI_CMD_SPEED_INCREASE; btn_event_ext = 0; }
		#endif

		if(ui_command==BYTEBEAT_UI_CMD_NEXT_SONG)
		{
			bytebeat_next_song();
		}
		if(ui_command==BYTEBEAT_UI_CMD_STEREO_PANNING)
		{
			bytebeat_stereo_paning();
			#ifdef BOARD_WHALE
			bytebeat_echo_on = (bytebeat_echo_cycle%(2*STEREO_MIXING_STEPS)) / STEREO_MIXING_STEPS;
			bytebeat_echo_cycle++;
			printf("bytebeat_engine(): Pannig step, echo = %d (ec=%d), paning step = %d\n", bytebeat_echo_on, bytebeat_echo_cycle, stereo_mixing_step);
			#else
			printf("bytebeat_engine(): Pannig step = %d\n", stereo_mixing_step);
			#endif
		}
		if(ui_command==BYTEBEAT_UI_CMD_SPEED_INCREASE)
		{
			if(bytebeat_speed>BYTEBEAT_SPEED_MAX)
			{
				if(bytebeat_speed>=16)
				{
					bytebeat_speed/=2;
				}
				else
				{
					bytebeat_speed--;
				}
				bytebeat_speed_div = 0;
			}
			printf("bytebeat_engine(): Speed up => %d\n", bytebeat_speed);
		}
		if(ui_command==BYTEBEAT_UI_CMD_SPEED_DECREASE)
		{
			if(bytebeat_speed<BYTEBEAT_SPEED_MIN)
			{
				if(bytebeat_speed>=8)
				{
					bytebeat_speed*=2;
				}
				else
				{
					bytebeat_speed++;
				}
				bytebeat_speed_div = 0;
			}
			printf("bytebeat_engine(): Slow down => %d\n", bytebeat_speed);
		}
		*/
	}

	#ifdef BYTEBEAT_MCLK
	stop_MCLK();
	#endif
}

/*
void bytebeat_next_song()
{
	bytebeat_song_ptr = 0;
	//bytebeat_speed_div = 0;
	bytebeat_song++;
	if(bytebeat_song==BYTEBEAT_SONGS)
	{
		bytebeat_song = 0;
	}
	//LED_R8_set_byte(1<<bytebeat_song);
}

void bytebeat_stereo_paning()
{
	//uint8_t stereo_mixing_step_indication[STEREO_MIXING_STEPS] = {0x18,0x24,0x42,0x81};

	stereo_mixing_step++;
	if(stereo_mixing_step==STEREO_MIXING_STEPS)
	{
		stereo_mixing_step = 0;
	}
	//LED_R8_set_byte(stereo_mixing_step_indication[stereo_mixing_step]);
}
*/

IRAM_ATTR int16_t bytebeat_echo(int16_t sample)
{
	if(echo_dynamic_loop_length)
	{
		//wrap the echo loop
		echo_buffer_ptr0++;
		if (echo_buffer_ptr0 >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr0 = 0;
		}

		echo_buffer_ptr = echo_buffer_ptr0 + 1;
		if (echo_buffer_ptr >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr = 0;
		}

		//add echo from the loop
		echo_mix_f = float(sample) + float(echo_buffer[echo_buffer_ptr]) * BYTEBEAT_ECHO_MIXING_FACTOR;

		if (echo_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER)
		{
			echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER;
		}

		if (echo_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER)
		{
			echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER;
		}

		sample = (int16_t)echo_mix_f;

		//store result to echo
		echo_buffer[echo_buffer_ptr0] = sample;
	}

	return sample;
}

void bytebeat_init()
{
	printf("bytebeat_init()\n");

	//echo_dynamic_loop_length = I2S_AUDIOFREQ / 128; //set default value (can be changed dynamically)
	echo_dynamic_loop_length = ECHO_BUFFER_LENGTH - 977*4;//43*4; //set default value (can be changed dynamically)
	//echo_buffer = (int16_t*)malloc(echo_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer

	/*
    //tt=0;
    bytebeat_song_ptr = 0;
    bytebeat_song = 5; //4;//7;//8;//7;//6;//0;
    patch = bytebeat_song;
    //bytebeat_speed_div = 0;

    bit1 = patch_bit1[patch];
    bit2 = patch_bit2[patch];
    */
	trigger_patch(PATCH_SILENCE);
	patch = PATCH_SILENCE;

    reset_arp_active_steps();

    //LED_R8_set_byte(1<<bytebeat_song);

	#ifdef BOARD_WHALE
    BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;
	#endif

    //layer = 7;
    //flash_map_samples();
    FLASH_SAMPLE_FORMAT = get_sample_format();
    BB_SHIFT_VOLUME = get_bb_volume();

    /*
    for(int i=0;i<NUM_PATCHES;i++)
    {
        layer7_patch_start[i] = SAMPLES_LENGTH/8*i;
        layer7_patch_length[i] = FLASH_SAMPLE_LENGTH_DEFAULT;
        layer7_patch_bitrate[i] = FLASH_SAMPLE_BITRATE_DEFAULT;
    }
    */
}

int leds_glowing[2] = {0,0};

IRAM_ATTR uint32_t bytebeat_next_sample()
{
	int tt[4];
	unsigned char s[4];
	float mix1 = 0, mix2 = 0;
	uint16_t sample1, sample2;
	//int16_t sample1i, sample2i;
	uint8_t flash_su;
	int8_t flash_s;
	uint32_t bb_sample;

	//static float s_lpf[4] = {0,0,0,0};
	//#define S_LPF_ALPHA	0.06f

	//float stereo_mixing[STEREO_MIXING_STEPS] = {0.5f,0.4f,0.2f,0}; //50, 60, 80 and 100% mixing ratio

	/*
	if (TIMING_EVERY_100_MS == 43) //10Hz
	//if (TIMING_EVERY_250_MS == 43) //4Hz
	{
		for(int i=0;i<4;i++)
		{
			s_lpf[i] = s_lpf[i] + S_LPF_ALPHA * (ir_res[i] - s_lpf[i]);

			//if(PARAMETER_CONTROL_SELECTED_IRS)
			//{
			if(ir_res[i] > IR_sensors_THRESHOLD_1)
			{
				sensor_p[i] = s_lpf[i]*100;
				//printf("SENSOR_THRESHOLD_ORANGE_1, ir_res[1] = %f, sensor_p[1] = %d\n", ir_res[1], sensor_p[1]);
			}
			else
			{
				sensor_p[i] = 0;
				//printf("SENSOR_THRESHOLD_ORANGE_1 NOT, sensor_p[1] = 0\n");
			}
			//}
		}
	}
	*/

	/*
	bytebeat_speed_div++;
	if(bytebeat_speed_div==bytebeat_speed)
	{
		bytebeat_speed_div = 0;
		*/

		bytebeat_song_ptr++;

		#ifdef BYTEBEAT_TEMPO_CORRECTION
		if(bytebeat_song_ptr%BYTEBEAT_TEMPO_CORRECTION==0) //tempo correction
		{
			bytebeat_song_ptr++;
		}
		#endif

		if(bytebeat_song_length!=BYTEBEAT_LENGTH_UNLIMITED)
		{
			if(bytebeat_song_ptr >= bytebeat_song_start_effective+bytebeat_song_length_effective)
			{
				bytebeat_song_ptr = bytebeat_song_start_effective;
			}
		}

		if(sensors_active)
		{
			if(sampleCounter % 100 == 0)
			{
				if((sensors_active&0x00000002) //right sensor active
				&& (light_sensor_results[0]>sensor_base))
				{
					bytebeat_song_length_effective = bytebeat_song_length - (int)((light_sensor_results[0]-sensor_base)/SENSOR_RANGE);
				}
				else
				{
					bytebeat_song_length_effective = bytebeat_song_length;
				}

				if((sensors_active&0x00000001) //left sensor active
				&& (light_sensor_results[1]>100))
				{
					bytebeat_song_start_effective = bytebeat_song_start + (int)light_sensor_results[1]/2;
				}
				else
				{
					bytebeat_song_start_effective = bytebeat_song_start;
				}
			}
		}
		else
		{
			bytebeat_song_length_effective = bytebeat_song_length;
			bytebeat_song_start_effective = bytebeat_song_start;
		}

		bytebeat_song_length_effective /= bytebeat_song_length_div;
		bytebeat_song_start_effective += bytebeat_song_pos_shift;

		scan_keypad();

		//------------------------------------------------------------------------------------------------------

		mix1 = 0;
		mix2 = 0;

		#include "bytebeat_songs.inc"

		//mix1 -= 128.0f;
		//mix2 -= 128.0f;

	//s[0] = (int16_t)(mix1*0.7f+mix2*0.3f);
	//s[1] = (int16_t)(mix2*0.7f+mix1*0.3f);
	//sample1 = (int16_t)(mix1*0.6f*BYTEBEAT_MIXING_VOLUME+mix2*0.4f*BYTEBEAT_MIXING_VOLUME);
	//sample2 = (int16_t)(mix1*0.4f*BYTEBEAT_MIXING_VOLUME+mix2*0.6f*BYTEBEAT_MIXING_VOLUME);
	//sample1 = (mix1*stereo_mixing[stereo_mixing_step]+mix2*(1.0f-stereo_mixing[stereo_mixing_step])) * BYTEBEAT_MIXING_VOLUME;
	//sample2 = (mix2*stereo_mixing[stereo_mixing_step]+mix1*(1.0f-stereo_mixing[stereo_mixing_step])) * BYTEBEAT_MIXING_VOLUME;

	if(layers_active&WAVETABLE_LAYERS_MASK)
	{
		sine_waves_next_sample(&sample1, &sample2);
		//printf("sine_waves_next_sample(&sample1, &sample2): %d, %d\n", sample1, sample2);

		//sample1<<=1;
		//sample2<<=1;
	}
	else
	{
		//sample1i = 0;
		//sample2i = 0;
		sample1 = 0;
		sample2 = 0;
	}

	//if(IS_FLASH_SAMPLE_LAYER && layer7_playhead>=0)
	if(layers_active&0x80) //if flash layer active
	{
		if(FLASH_SAMPLE_FORMAT!=1)
		{
			//flash_s = 0;
			flash_su = 128;
		}
		else
		{
			flash_su = 0;
		}

		if(layer7_playhead>=0)
		{
			/*
			//((char*)&sample1i)[0] = ((char*)((unsigned int)samples_ptr1 + layer7_playhead))[0];
			((char*)&sample1)[0] = ((char*)((unsigned int)samples_ptr1 + layer7_playhead))[0];
			//flash_s = ((char*)((unsigned int)samples_ptr1 + layer7_playhead))[0];
			//sample1i = sample1;// - 32768;
			sample1 += 32768;
			*/
			flash_su = ((unsigned char*)((unsigned int)samples_ptr1 + layer7_playhead))[0];
			//flash_s += 128;

			//printf("[1]: sample1i = %d, sample1 = %d\n", sample1i, sample1);
			if((layer7_grain_bitrate<=0) || (layer7_grain_bitrate>0 && sampleCounter%layer7_grain_bitrate==0))
			{
				layer7_playhead++;
				if(layer7_grain_bitrate<=0)
				{
					layer7_playhead+=1-layer7_grain_bitrate;
					layer7_grain_length-=1-layer7_grain_bitrate;
				}

				if(--layer7_grain_length<=-1)
				{
					layer7_grain_length = -1;
					layer7_playhead = -1;
					if(!encoder_blink && menu_function==MENU_PLAY) //to avoid turning a blinking LED off, if assigning key-to-key
					{
						LEDS_BLUE_OFF;
					}
				}
			}
		}

		//sample1i *= 8;
		//sample2i = sample1i;
		//sample2 = sample1;
		//printf("[2]: sample1i = %d\n", sample1i);

		//mix1 += (((float)flash_s)-128)/2;
		//mix2 += (((float)flash_s)-128)/2;
		//mix1 += (((float)flash_s)/2)-128;
		//mix2 += (((float)flash_s)/2)-128;
		//mix1 += (((float)flash_s)/2);
		//mix2 += (((float)flash_s)/2);

		//if(mix1<-127) mix1=-127;
		//if(mix1>127) mix1=127;
		//if(mix2<-127) mix2=-127;
		//if(mix2>127) mix2=127;

		//sample1i += flash_s;
		//sample2i = sample1i;

		/*
		sample1 += 128;
		sample2 += 128;

		//printf("sample1[1]=%d",sample1);
		if(sample1>255)sample1=255;
		if(sample2>255)sample2=255;
		//printf("sample1[2]=%d",sample1);
		*/

		if(FLASH_SAMPLE_FORMAT==1)
		{
			flash_s = flash_su;// mod format (signed) https://www.fileformat.info/format/mod/spec/3bc11a4842e342498a6230e60187b463/view.htm
		}
		else
		{
			flash_s = flash_su + 128; //s3m format (unsigned samples) https://moddingwiki.shikadi.net/wiki/S3M_Format
		}

		//this is only needed for signed-8bit (sound forge), mod files sound better without
		//flash_s += 128;

		#define SAMPLE_FORMAT_SHIFT	(128) //32768

		if(FLASH_SAMPLE_BOOST_VOLUME==0)
		{
			//out_sample1[0] = sample1;//<<2;
			//out_sample2[0] = sample2;//<<2;
			sample1 += ((uint16_t)flash_s)+SAMPLE_FORMAT_SHIFT;
			sample2 += ((uint16_t)flash_s)+SAMPLE_FORMAT_SHIFT;
		}
		else if(FLASH_SAMPLE_BOOST_VOLUME>0)
		{
			//out_sample1[0] = sample1<<WAVESAMPLE_BOOST_VOLUME;
			//out_sample2[0] = sample2<<WAVESAMPLE_BOOST_VOLUME;
			sample1 += (((uint16_t)flash_s)+SAMPLE_FORMAT_SHIFT)<<FLASH_SAMPLE_BOOST_VOLUME;
			sample2 += (((uint16_t)flash_s)+SAMPLE_FORMAT_SHIFT)<<FLASH_SAMPLE_BOOST_VOLUME;
		}
		else
		{
			//out_sample1[0] = sample1>>(-WAVESAMPLE_BOOST_VOLUME);
			//out_sample2[0] = sample2>>(-WAVESAMPLE_BOOST_VOLUME);
			sample1 += (((uint16_t)flash_s)+SAMPLE_FORMAT_SHIFT)>>(-FLASH_SAMPLE_BOOST_VOLUME);
			sample2 += (((uint16_t)flash_s)+SAMPLE_FORMAT_SHIFT)>>(-FLASH_SAMPLE_BOOST_VOLUME);
		}
	}

	/*
	if(sampleCounter%1000==0)
	{
		printf("%f/%f\n", mix1, mix2);
	}
	*/

	//mix1 += 128.0f;
	//mix2 += 128.0f;

	//sample1 = 32768;
	//sample2 = 32768;
	//sample2 = sample1;

	//sample1 += (((uint16_t)sample1i)+32768);
	//sample2 += (((uint16_t)sample2i)+32768);
	//sample1 += (((uint16_t)sample1i));
	//sample2 += (((uint16_t)sample2i));

	//printf("[3]: sample1 = %d ------------\n", sample1);

	//sample1 *= 2;
	//sample2 *= 2;

	if(layers_active&0x01)
	{
		sample1 += ((uint16_t)mix1);
		sample2 += ((uint16_t)mix2);
		//sample1i += ((int16_t)mix1);
		//sample2i += ((int16_t)mix2);
	}

	//------------------------------------------------------------------------------------------------------

	#ifdef DEBUG_SAMPLE_LEVELS
	if(sampleCounter%1000==0)
	{
		printf("%d	%d\n", sample1, sample2);
	}
	#endif

	if(bytebeat_echo_on)
	{
		sample1 = bytebeat_echo(sample1);
		sample2 = bytebeat_echo(sample2);
	}

	bb_sample = (sample1 << BB_SHIFT_VOLUME) & 0x0000ffff;
	bb_sample <<= 16;
	bb_sample |= (sample2 << BB_SHIFT_VOLUME) & 0x0000ffff;

	//bb_sample = (sample1i << BB_SHIFT_VOLUME) & 0x0000ffff;
	//bb_sample <<= 16;
	//bb_sample |= (sample2i << BB_SHIFT_VOLUME) & 0x0000ffff;

	//bb_sample = sample1i;
	//bb_sample <<= 16;
	//bb_sample |= sample2i;

	/*
	if(bytebeat_song_ptr%100<50)
	{
		bb_sample = 0xff000000;
	}
	else
	{
		bb_sample = 0x00000000;
	}
	*/

	//return ((uint32_t)s[0])<<7|(((uint32_t)s[1])<<23);
	return bb_sample;// << 1;
}

int is_sequence_allocated(int position)
{
	return (seq_pattern[position] != NULL);
}

void init_sequence(int position)
{
	if(position<NUM_SEQUENCES)
	{
		if(is_sequence_allocated(position))
		{
			#ifdef DEBUG_OUTPUT
			printf("init_sequence(%d): position already allocated (0x%x)\n", position, (unsigned int)seq_pattern[position]);
			#endif
		}
		else
		{
			//allocate space for maximum possible amount of steps
			seq_pattern[position] = (int8_t*)malloc(SEQ_STEPS_MAX*sizeof(int8_t)); //int8_t* seq_pattern[NUM_SEQUENCES];
			#ifdef DEBUG_OUTPUT
			printf("init_sequence(%d): position allocated => 0x%x\n", position, (unsigned int)seq_pattern[position]);
			#endif
			memset(seq_pattern[position],0,SEQ_STEPS_MAX*sizeof(int8_t));
			seq_filled_steps[position] = 0;
		}
	}
	else
	{
		printf("init_sequence(%d): position out of range\n", position);
	}
}

void deinit_sequence(int position)
{
	if(position<NUM_SEQUENCES)
	{
		if(seq_pattern[position])
		{
			free(seq_pattern[position]);
			seq_pattern[position] = NULL;
			#ifdef DEBUG_OUTPUT
			printf("deinit_sequence(%d): position freed\n", position);
			#endif
		}
		else
		{
			#ifdef DEBUG_OUTPUT
			printf("deinit_sequence(%d): position already free\n", position);
			#endif
		}
	}
	else
	{
		printf("deinit_sequence(%d): position out of range\n", position);
	}
}

void animate_sequencer()
{
	if(sequencer/* && seq_running*/) //sequencer
	{
		led_disp[LEDS_ORANGE2] = seq_step_ptr%seq_steps_r1[sequencer-1]+1;
		led_disp[LEDS_ORANGE1] = (seq_step_ptr/seq_steps_r1[sequencer-1])%seq_steps_r2[sequencer-1]+1;
		//printf("animate_sequencer(steps=%dx%d): ptr=%d, disp=%d,%d\n", seq_steps_r1[sequencer-1], seq_steps_r2[sequencer-1], seq_step_ptr, led_disp[LEDS_ORANGE1], led_disp[LEDS_ORANGE2]);
	}
	else //arpeggiator
	{
		if(sequencer_blink) //possibly never used
		{
			led_disp[LEDS_ORANGE2] = sequencer_counter%4+1 + LEDS_BLINK;
			led_disp[LEDS_ORANGE1] = (sequencer_counter/4)%4+1 + LEDS_BLINK;
		}
		else
		{
			led_disp[LEDS_ORANGE2] = sequencer_counter%4+1;
			led_disp[LEDS_ORANGE1] = (sequencer_counter/4)%4+1;
		}
	}

	sequencer_counter++;
}

void start_stop_sequencer(int sequencer)
{
	sequencer_blink = 0; //possibly not used
	sequencer_counter = 0;
	seq_step_ptr = SEQ_STEP_START;
	sampleArpSeq = 0;
	arp_seq_dir = 1;
	seq_octave_effective = seq_range_from;

	if(sequencer<=0 || seq_filled_steps[sequencer-1]<=0)
	{
		seq_running = 0;
		if(seq_layer==0)
		{
			trigger_patch(PATCH_SILENCE);
		}
	}
	else if(SEQUENCE_COMPLETE(sequencer))
	{
		seq_running = 1;
	}
	else //some sequence defined but incomplete
	{
		seq_running = 0;
		if(seq_layer==0)
		{
			trigger_patch(PATCH_SILENCE);
		}
	}

	#ifdef DEBUG_OUTPUT
	printf("start_stop_sequencer(%d): seq_running => %d\n", sequencer, seq_running);
	#endif
}

void reset_arp_active_steps()
{
	arp_active_steps = 0;

	for(int i=0;i<ARP_MAX_STEPS;i++)
	{
		if(arp_pattern[i])
		{
			arp_active_steps++;
		}
	}

	#ifdef DEBUG_OUTPUT
	printf("reset_arp_active_steps(): arp_active = %d\n", arp_active_steps);
	#endif
}

void assign_current_patch_params()
{
	#ifdef DEBUG_OUTPUT
	printf("assign_current_patch_params(): patch = %d\n", patch);
	#endif

	if(patch<0 || patch >= NUM_PATCHES)
	{
		printf("assign_current_patch_params(): out of range, not assigning\n");
		return;
	}

	patch_song_start[patch] = bytebeat_song_start;
	patch_song_length[patch] = bytebeat_song_length;
	patch_song[patch] = bytebeat_song;

	patch_var1[patch] = var_p[0];
	patch_var2[patch] = var_p[1];
	patch_var3[patch] = var_p[2];
	patch_var4[patch] = var_p[3];
	patch_bit1[patch] = bit1;
	patch_bit2[patch] = bit2;
}

void layer7_trigger_sample(int note)
{
	#ifdef DEBUG_OUTPUT
	printf("layer7_trigger_sample(): note = %d\n", note);
	#endif

	if(note>=0 && note<NUM_PATCHES)
	{
		layer7_playhead = layer7_patch_start[note];
		layer7_grain_length = layer7_patch_length[note];
		layer7_grain_bitrate = layer7_patch_bitrate[note];
		layer7_last_key = note;

		#ifdef DEBUG_OUTPUT
		printf("layer7_trigger_sample(): layer7_playhead = %d, layer7_grain_length = %d, layer7_grain_bitrate = %d\n", layer7_playhead, layer7_grain_length, layer7_grain_bitrate);
		#endif
	}
}
