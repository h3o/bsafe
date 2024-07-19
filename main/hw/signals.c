/*
 * signals.c
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

#include "signals.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//-------------------------- sample and echo buffers -------------------

float sample_mix;

uint32_t sample32;
uint32_t sampleCounter = 0, sampleArpSeq = 0;

#ifdef ECHO_BUFFER_STATIC
int16_t echo_buffer[ECHO_BUFFER_LENGTH];	//the buffer is allocated statically
#else
int16_t *echo_buffer = NULL;	//the buffer is allocated dynamically
#endif

int echo_buffer_ptr0, echo_buffer_ptr;
int echo_dynamic_loop_length = ECHO_BUFFER_LENGTH; //default length
int echo_dynamic_loop_current_step = 0;
float echo_mix_f;

const int echo_dynamic_loop_steps[ECHO_DYNAMIC_LOOP_STEPS] = {
	ECHO_BUFFER_LENGTH,		//default length, 1.5 sec (I2S_AUDIOFREQ * 3 / 2)
	(AUDIOFREQ),			//one second
	(AUDIOFREQ / 3 * 2),	//short delay
	(AUDIOFREQ / 2),		//short delay
	//(AUDIOFREQ / 3),		//short delay
	(AUDIOFREQ / 4),		//short delay
	(AUDIOFREQ / 6),		//reverb
	680, 					//flanger
	0};

float SAMPLE_VOLUME = SAMPLE_VOLUME_DEFAULT;
float limiter_coeff = DYNAMIC_LIMITER_COEFF_DEFAULT;

//---------------------------------------------------------------------------

double static b_noise = NOISE_SEED;
uint32_t random_value;

void reset_pseudo_random_seed()
{
	b_noise = NOISE_SEED;
}

void set_pseudo_random_seed(double new_value)
{
	//printf("set_pseudo_random_seed(%f)\n",new_value);
	b_noise = new_value;
}

float PseudoRNG1a_next_float() //returns random float between -0.5f and +0.5f
{
/*
	b_noise = b_noise * b_noise;
	int i_noise = b_noise;
	b_noise = b_noise - i_noise;

	float b_noiseout;
	b_noiseout = b_noise - 0.5;

	b_noise = b_noise + 19;

	return b_noiseout;
*/
	b_noise = b_noise + 19;
	b_noise = b_noise * b_noise;
	int i_noise = b_noise;
	b_noise = b_noise - i_noise;
	return b_noise - 0.5;
}

/*
float PseudoRNG1b_next_float()
{
	double b_noiselast = b_noise;
	b_noise = b_noise + 19;
	b_noise = b_noise * b_noise;
	b_noise = (b_noise + b_noiselast) * 0.5;
	b_noise = b_noise - (int)b_noise;

	return b_noise - 0.5;
}

uint32_t PseudoRNG2_next_int32()
{
	//http://musicdsp.org/showone.php?id=59
	//Type : Linear Congruential, 32bit
	//References : Hal Chamberlain, "Musical Applications of Microprocessors" (Posted by Phil Burk)
	//Notes :
	//This can be used to generate random numeric sequences or to synthesise a white noise audio signal.
	//If you only use some of the bits, use the most significant bits by shifting right.
	//Do not just mask off the low bits.

	//Calculate pseudo-random 32 bit number based on linear congruential method.

	//Change this for different random sequences.
	static unsigned long randSeed = 22222;
	randSeed = (randSeed * 196314165) + 907633515;
	return randSeed;
}
*/

void new_random_value()
{
	float r = PseudoRNG1a_next_float();
	memcpy(&random_value, &r, sizeof(random_value));
}

int fill_with_random_value(char *buffer)
{
	float r = PseudoRNG1a_next_float();
	memcpy(buffer, &r, sizeof(r));
	return sizeof(r);
}

//this seems to be NOT a more optimal way of passing the value
void PseudoRNG_next_value(uint32_t *buffer) //load next random value to the variable
{
	b_noise = b_noise * b_noise;
	int i_noise = b_noise;
	b_noise = b_noise - i_noise;
	float b_noiseout = b_noise - 0.5;
	b_noise = b_noise + NOISE_SEED;
	memcpy(buffer, &b_noiseout, sizeof(b_noiseout));
}

//uint8_t test[10000]; //test for how much static memory is left

void init_echo_buffer()
{
	#ifdef ECHO_BUFFER_STATIC
	//printf("init_echo_buffer(): buffer allocated as static array\n");
	#else
	printf("init_echo_buffer(): free heap = %u, allocating = %u\n", xPortGetFreeHeapSize(), ECHO_BUFFER_LENGTH * sizeof(int16_t));
	echo_buffer = (int16_t*)malloc(ECHO_BUFFER_LENGTH * sizeof(int16_t));
	printf("init_echo_buffer(): echo_buffer = %x\n", (unsigned int)echo_buffer);
	if(echo_buffer==NULL)
	{
		printf("init_echo_buffer(): could not allocate buffer of size %d bytes\n", ECHO_BUFFER_LENGTH * sizeof(int16_t));
		while(1);
	}
	#endif
}

void deinit_echo_buffer()
{
	#ifdef ECHO_BUFFER_STATIC
	//printf("deinit_echo_buffer(): buffer allocated as static array\n");
	#else
	if(echo_buffer!=NULL)
	{
		free(echo_buffer);
		echo_buffer = NULL;
		printf("deinit_echo_buffer(): memory freed\n");
	}
	else
	{
		printf("deinit_echo_buffer(): buffer not allocated\n");
	}
	#endif
}
