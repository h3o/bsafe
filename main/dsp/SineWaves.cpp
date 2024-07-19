/*
 * SineWaves.cpp
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

#include <SineWaves.h>
#include <Bytebeat.h>
#include <hw/signals.h>
#include <hw/gpio.h>
#include <hw/keys.h>
#include <string.h>
#include <math.h>

//#define DEBUG_OUTPUT

double w_step/* = W_STEP_DEFAULT*/, w_ptr = 0, env_amp = 0;
uint8_t *sinetable, *sawtable, *squaretable, *multitable, *noisetable;
float *envtable;
int e_ptr = -1, w_type, e_div, wt_ca;
int note_triggered = -1, last_note_triggered = 0, wt_octave_shift = 0, wt_cycle_alignment = WT_CYCLE_ALIGNMENT_DEFAULT;

#ifdef SAMPLING_RATE_32KHZ
#ifdef WAVETABLE_SIZE_DOUBLE
uint16_t mini_piano_tuning[MINI_PIANO_KEYS] = {4400,4939,5233,5873,6593,6985,8306,8800}; //default scale for 32kHz sampling rate
#else
uint16_t mini_piano_tuning[MINI_PIANO_KEYS] = {2200,2469,2616,2937,3296,3492,4153,4400}; //default scale for 32kHz sampling rate
#endif
#else

#ifdef WAVETABLE_SIZE_DOUBLE
//higher octave, after doubling the wavetable size
uint16_t mini_piano_tuning[MINI_PIANO_KEYS] = {4297,4823,5110,5736,6438,6821,8111,8594}; //default scale for 32.768kHz sampling rate
#else
//default scale for 32.768kHz sampling rate
//uint16_t mini_piano_tuning[MINI_PIANO_KEYS] = {2148,2412,2555,2868,3219,3410,4056,4297}; //equal temperament
#define MINI_PIANO_TUNING_DEFAULT {2159,2424,2593,2882,3249,3465,4110,4297}
uint16_t mini_piano_tuning[MINI_PIANO_KEYS] = MINI_PIANO_TUNING_DEFAULT; //corrected by ear
#endif

#endif

uint8_t mini_piano_waves[MINI_PIANO_KEYS];
int32_t mini_piano_tuning_enc;

float RND_LPF_ALPHA, rnd_signal_filtered;
int8_t rnd_envelope_div[MINI_PIANO_KEYS];

int8_t WAVESAMPLE_BOOST_VOLUME = WAVESAMPLE_BOOST_VOLUME_DEFAULT;

void sine_waves_init()
{
	//printf("sine_waves_init()\n");

	//int16_t *sinetable = (int16_t*)malloc(SINETABLE_SIZE*sizeof(int16_t));
	sinetable = (uint8_t*)malloc(WAVETABLE_SIZE*sizeof(uint8_t));
	sawtable = (uint8_t*)malloc(WAVETABLE_SIZE*sizeof(uint8_t));
	squaretable = (uint8_t*)malloc(WAVETABLE_SIZE*sizeof(uint8_t));
	multitable = (uint8_t*)malloc(WAVETABLE_SIZE*sizeof(uint8_t));
	noisetable = (uint8_t*)malloc(WAVETABLE_SIZE*sizeof(uint8_t) + 3); //extra bytes for random filler function

	double v;
	int16_t d;

	printf("sine_waves_init(): generating wavetables...\n");

	for(int i=0;i<WAVETABLE_SIZE;i++)
	{
		v = sin(2*M_PI*(double)i/double(WAVETABLE_SIZE));

		//for uint16_t:
		//d = (v+1.0f) * SINETABLE_AMP; //gives sine sound but with initial click
		//d = (int16_t)(v * SINETABLE_AMP); //gives clipped sound

		//for int16_t:
		d = v * SINETABLE_AMP + 128;

		//printf("%f/%d\t",v,d);
		//printf("%d:%d,",i,d);
		//printf("%d,",d);
		sinetable[i] = d;
		sawtable[i] = SAWTABLE_DC_OFFSET + SAWTABLE_AMP * (double)i/double(WAVETABLE_SIZE);
		squaretable[i] = SQUARETABLE_DC_OFFSET + ((i<WAVETABLE_SIZE/2)?0:SQUARETABLE_AMP);

		v = cos(2*M_PI*(double)i/double(WAVETABLE_SIZE));
		d = v * SINETABLE_AMP;

		multitable[i] = d + SAWTABLE_AMP * (double)i/double(WAVETABLE_SIZE)
						  + SAWTABLE_AMP * (double)(i+WAVETABLE_SIZE/13)/double(WAVETABLE_SIZE)
						  + SAWTABLE_AMP * (double)(i+WAVETABLE_SIZE/3)/double(WAVETABLE_SIZE)
						  + SAWTABLE_AMP * (double)(i+WAVETABLE_SIZE/7)/double(WAVETABLE_SIZE);
		//if(multitable[i]>255){ multitable[i] -= 255; }

		//printf("wavetables[%d]:		%d	%d	%d\n", i, sinetable[i], sawtable[i], squaretable[i]);
		noisetable[i] = d;
		if(i%4==0) fill_with_random_value((char*)(noisetable+i));
	}
	//printf("\n");

	//uint16_t *envtable = (uint16_t*)malloc(ENVTABLE_SIZE*sizeof(uint16_t));
	//uint8_t *envtable = (uint8_t*)malloc(ENVTABLE_SIZE*sizeof(uint8_t));
	envtable = (float*)malloc(ENVTABLE_SIZE*sizeof(float));

	printf("sine_waves_init(): generating envtable...\n");
	for(int i=0;i<ENVTABLE_SIZE;i++)
	{
		v = pow(1.0f-ENV_DECAY_FACTOR,1.0f+(double)i/ENV_DECAY_STEP_DIV);
		//d = v * ENVTABLE_AMP + ENVTABLE_OFFSET;
		//printf("%d:%d,",i,d);
		//printf("%d,",d);
		//envtable[i] = d;
		envtable[i] = v * ENVTABLE_AMP + ENVTABLE_OFFSET;
		//printf("%f,",envtable[i]);
	}
	//printf("\n");

	for(int i=0;i<MINI_PIANO_KEYS;i++)
	{
		//mini_piano_tuning[i] = MINI_PIANO_TUNING_BASE * (W_STEP_DEFAULT + i * W_STEP_DEFAULT);
		//mini_piano_tuning[i] /= 2; //shift by one octave down
		mini_piano_waves[i] = MINI_PIANO_WAVE_DEFAULT;
		rnd_envelope_div[i] = 1;
	}
}

void sine_waves_next_sample(uint16_t *out_sample1, uint16_t *out_sample2)
{
	uint16_t sample1, sample2;
	int i_ptr;

	if (TIMING_EVERY_25_MS == 31) //40Hz
	{
		if(note_triggered>0)
		{
			//w_step = W_STEP_DEFAULT + note_triggered * W_STEP_DEFAULT;

			if((sensors_wt_layers&0x02) && (light_sensor_results[0]>sensor_base)) //right sensor active
			{
				//one octave up per whole range: ls_coeff = <1.0f ... 2.0f>
				double rs_coeff = 1.0f + (double)((light_sensor_results[0]-sensor_base)/(SENSOR_RANGE*4095/SENSOR_RANGE_DEFAULT-SENSOR_BASE_DEFAULT));

				w_step = (double)(mini_piano_tuning[note_triggered-1]) / (double)MINI_PIANO_TUNING_BASE * rs_coeff;
				#ifdef DEBUG_OUTPUT
				printf("note_triggered: %d, rs_coeff: %f, w_step: %f\n", note_triggered, rs_coeff, w_step);
				#endif
			}
			else
			{
				w_step = (double)mini_piano_tuning[note_triggered-1] / (double)MINI_PIANO_TUNING_BASE;
				#ifdef DEBUG_OUTPUT
				printf("note_triggered: %d, w_step: %f\n", note_triggered, w_step);
				#endif
			}

			if(wt_octave_shift!=0)
			{
				int divider;
				if(wt_octave_shift>=0)
				{
					divider = wt_octave_shift+1;
				}
				else if(wt_octave_shift<=-2)
				{
					divider = -4;
				}
				else
				{
					divider = wt_octave_shift-1;
				}

				if(divider>0)
				{
					w_step *= (double)divider;
				}
				else
				{
					w_step /= (double)(-divider);
				}
			}

			w_ptr = 0;
			e_ptr = 0;
			env_amp = 1.0f;
			wt_ca = 0;

			w_type = mini_piano_waves[note_triggered-1];

			if(w_type==MINI_PIANO_WAVE_DEFAULT)
			{
				if(layers_active&0x02) w_type = MINI_PIANO_WAVE_SINE;
				else if(layers_active&0x04) w_type = MINI_PIANO_WAVE_SAW;
				else if(layers_active&0x08) w_type = MINI_PIANO_WAVE_SQUARE;
				else if(layers_active&0x10) w_type = MINI_PIANO_WAVE_MULTI;
				else if(layers_active&0x20) w_type = MINI_PIANO_WAVE_NOISE;
				else if(layers_active&0x40) w_type = MINI_PIANO_WAVE_RNG;
			}

			if(w_type==MINI_PIANO_WAVE_RNG)// || (w_type==MINI_PIANO_WAVE_DEFAULT && (layers_active&0x40)))
			{
				//RND_LPF_ALPHA = ((double)(mini_piano_tuning[note_triggered-1]-MINI_PIANO_TUNING_MIN)) / (double)MINI_PIANO_TUNING_MAX;
				RND_LPF_ALPHA = ((double)((mini_piano_tuning[note_triggered-1]-MINI_PIANO_TUNING_MIN)>>8)) / (MINI_PIANO_TUNING_MAX>>8);
				rnd_signal_filtered = 0;

				e_div = rnd_envelope_div[note_triggered-1];
				if(e_div<1) { e_div = 1; rnd_envelope_div[note_triggered-1] = e_div; }
				if(e_div>WAVE_RNG_DIVIDER_MAX) { e_div = WAVE_RNG_DIVIDER_MAX; rnd_envelope_div[note_triggered-1] = e_div; }
			}
			else
			{
				e_div = 1;

				if((sensors_wt_layers&0x01) && (light_sensor_results[1]>sensor_base)) //left sensor active
				{
					//ls_coeff = <0.0f ... 1.0f>
					double ls_coeff = (double)((light_sensor_results[1]-sensor_base)/(SENSOR_RANGE*4095/SENSOR_RANGE_DEFAULT-SENSOR_BASE_DEFAULT));

					/*
					if(w_type==MINI_PIANO_WAVE_DEFAULT)
					{
						if(layers_active&0x02) w_type = MINI_PIANO_WAVE_SINE;
						else if(layers_active&0x04) w_type = MINI_PIANO_WAVE_SAW;
						else if(layers_active&0x08) w_type = MINI_PIANO_WAVE_SQUARE;
						else if(layers_active&0x10) w_type = MINI_PIANO_WAVE_MULTI;
						else if(layers_active&0x20) w_type = MINI_PIANO_WAVE_NOISE;
					}
					*/

					//cycle through all waveforms (except noise) and back to the original one
					//w_type = 1 + (w_type + (int)floor(ls_coeff * 5.0f)) % 5;

					//cycle through all waveforms (except noise) but not back to the original one
					w_type = 1 + (w_type + (int)floor(ls_coeff * 4.0f)) % 5;

					#ifdef DEBUG_OUTPUT
					printf("note_triggered: %d, ls_coeff: %f, w_type: %d\n", note_triggered, ls_coeff, w_type);
					#endif

				}

				if(wt_cycle_alignment & 0x01) //left side (layers 2-4) aligned
				{
					if(w_type==MINI_PIANO_WAVE_SINE || w_type==MINI_PIANO_WAVE_SAW || w_type==MINI_PIANO_WAVE_SQUARE)
					{
						wt_ca = 1;
					}
				}

				if(wt_cycle_alignment & 0x02) //right side (layers 5-6) aligned
				{
					if(w_type==MINI_PIANO_WAVE_MULTI || w_type==MINI_PIANO_WAVE_NOISE)
					{
						wt_ca = 1;
					}
				}
			}

			#ifdef DEBUG_OUTPUT
			printf("note_triggered: %d, w_type: %d, RND_LPF_ALPHA: %f, e_div: %d, layers_active = %02x, layer = %d\n", note_triggered, w_type, RND_LPF_ALPHA, e_div, layers_active, layer);
			#endif

			note_triggered = -1;
		}
	}

	if(e_ptr>=0)
	{
		i_ptr = lround(w_ptr);
		//sample_mix = ((float)sinetable[i_ptr])/(float)4;

		//if(w_type)
		//{
			if(w_type==MINI_PIANO_WAVE_SINE) sample_mix = ((float)sinetable[i_ptr]) * env_amp;
			else if(w_type==MINI_PIANO_WAVE_SAW) sample_mix = ((float)sawtable[i_ptr]) * env_amp;
			else if(w_type==MINI_PIANO_WAVE_SQUARE) sample_mix = ((float)squaretable[i_ptr]) * env_amp;
			else if(w_type==MINI_PIANO_WAVE_MULTI) sample_mix = ((float)multitable[i_ptr]) * env_amp;
			else if(w_type==MINI_PIANO_WAVE_NOISE) sample_mix = ((float)noisetable[i_ptr]) * env_amp;
			else if(w_type==MINI_PIANO_WAVE_RNG)
			{
				new_random_value();
				//sample_mix = ((float)(random_value&0xff)) * env_amp;
				rnd_signal_filtered += RND_LPF_ALPHA * ((((float)(random_value&0xff)) * env_amp) - rnd_signal_filtered);
				sample_mix = rnd_signal_filtered;
			}
		/*
		}
		else //default wave as defined per layer
		{
			//sample_mix = 0;
			if(layers_active&0x02) sample_mix = ((float)sinetable[i_ptr]) * env_amp;;
			else if(layers_active&0x04) sample_mix = ((float)sawtable[i_ptr]) * env_amp;
			else if(layers_active&0x08) sample_mix = ((float)squaretable[i_ptr]) * env_amp;
			else if(layers_active&0x10) sample_mix = ((float)multitable[i_ptr]) * env_amp;
			else if(layers_active&0x20) sample_mix = ((float)noisetable[i_ptr]) * env_amp;
			else if(layers_active&0x40)
			{
				new_random_value();
				//sample_mix = ((float)(random_value&0xff)) * env_amp;
				rnd_signal_filtered += RND_LPF_ALPHA * ((((float)(random_value&0xff)) * env_amp) - rnd_signal_filtered);
				sample_mix = rnd_signal_filtered;
			}
		}
		*/

		w_ptr += w_step;
		if(w_ptr > WAVETABLE_SIZE)
		{
			if(wt_ca)
			{
				w_ptr -= (double)WAVETABLE_SIZE;
			}
			else
			{
				w_ptr = 0;
			}
		}

		//sample32 = ((((int16_t)(sample_mix))>>8)&0x00ff) << 16;
		//sample32 += (((int16_t)(sample_mix))>>8)&0x00ff;

		sample1 = sample_mix;
		sample2 = sample_mix;

		//printf("sample_mix=%f,sample1[0]=%d",sample_mix,sample1);
	}
	else
	{
		sample1 = 0;
		sample2 = 0;
	}

	sample1 += 128;
	sample2 += 128;

	//printf("sample1[1]=%d",sample1);
	if(sample1>255)sample1=255;
	if(sample2>255)sample2=255;
	//printf("sample1[2]=%d",sample1);

	if(WAVESAMPLE_BOOST_VOLUME==0)
	{
		out_sample1[0] = sample1;//<<2;
		out_sample2[0] = sample2;//<<2;
	}
	else if(WAVESAMPLE_BOOST_VOLUME>0)
	{
		out_sample1[0] = sample1<<WAVESAMPLE_BOOST_VOLUME;
		out_sample2[0] = sample2<<WAVESAMPLE_BOOST_VOLUME;
	}
	else
	{
		out_sample1[0] = sample1>>(-WAVESAMPLE_BOOST_VOLUME);
		out_sample2[0] = sample2>>(-WAVESAMPLE_BOOST_VOLUME);
	}

	/*
	if(bytebeat_echo_on)
	{
		sample1 = bytebeat_echo(sample1);
		sample2 = bytebeat_echo(sample2);
	}

	sample32 = (sample1 << BB_SHIFT_VOLUME) & 0x0000ffff;
	sample32 <<= 16;
	sample32 |= (sample2 << BB_SHIFT_VOLUME) & 0x0000ffff;
	//sample32 = sample1 & 0x0000ffff;
	//sample32 <<= 16;
	//sample32 |= sample2 & 0x0000ffff;
	*/

	//printf("sample32=%x\n",sample32);

	//sample32 = ((uint16_t)(sample_mix)) << 16;
	//sample32 += (uint16_t)(sample_mix);

	//if(TIMING_EVERY_10_MS == 37)
	if(TIMING_EVERY_2_MS == 13)
	{
		//printf(".\n");
		if(e_ptr>=0)
		{
			//printf("+\n");

			//if(w_type || (layers_active&0x40))
			if(w_type==MINI_PIANO_WAVE_RNG || (w_type==MINI_PIANO_WAVE_DEFAULT && (layers_active&0x40)))
			{
				e_ptr += e_div;
			}
			else
			{
				e_ptr++;
			}

			if(e_ptr>=ENVTABLE_SIZE-1)
			{
				//printf("[e]\n");
				e_ptr = -1;
				env_amp = ENVTABLE_OFFSET;//0;
				if(!encoder_blink && menu_function==MENU_PLAY) //to avoid turning a blinking LED off, if assigning key-to-key
				{
					LEDS_BLUE_OFF;
				}
			}
			else
			{
				env_amp = envtable[e_ptr];// / (double)(ENVTABLE_AMP+ENVTABLE_OFFSET);
			}
		}
	}
}

void sine_waves_stop_sound()
{
	e_ptr = -1;
}

void sine_waves_deinit()
{
	free(sinetable);
}

void reset_higher_layers(int reset_mini_piano, int reset_samples)
{
	if(reset_mini_piano)
	{
		printf("reset_higher_layers(): resetting mini piano variables\n");

		uint16_t tmp[MINI_PIANO_KEYS] = {2159,2424,2593,2882,3249,3465,4110,4297};
		memcpy(mini_piano_tuning, tmp, sizeof(mini_piano_tuning));

		note_triggered = -1;
		last_note_triggered = 0;
		wt_octave_shift = 0;
		wt_cycle_alignment = WT_CYCLE_ALIGNMENT_DEFAULT;
		WAVESAMPLE_BOOST_VOLUME = WAVESAMPLE_BOOST_VOLUME_DEFAULT;

		memset(mini_piano_waves,0,sizeof(mini_piano_waves));
		memset(rnd_envelope_div,0,sizeof(rnd_envelope_div));
	}

	if(reset_samples)
	{
		printf("reset_higher_layers(): resetting flash sample variables\n");
		layer7_patch_modified = 0;
		layer7_samples_delete = 1; //schedule to delete them at sample save

		for(int i=0;i<NUM_PATCHES;i++)
		{
			layer7_patch_start[i] = SAMPLES_LENGTH/8*i;
			layer7_patch_length[i] = FLASH_SAMPLE_LENGTH_DEFAULT;
			layer7_patch_bitrate[i] = FLASH_SAMPLE_BITRATE_DEFAULT;
		}
	}
}
