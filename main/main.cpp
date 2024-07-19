/*
 * main.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 17 Aug 2020
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

//#define INIT_ENCODERS_SENSORS_FOR_SELF_TEST

#include "board.h"
#include "main.h"

extern "C" void app_main(void)
{
	printf("The mighty BSAFE starting, Korean Hroparts encoders rolling...\n");
	printf("Running on core ID=%d\n", xPortGetCoreID());

	//uint8_t factory_MAC[8];
	//esp_efuse_mac_get_default(factory_MAC);
	//printf("Factory MAC = %02x:%02x:%02x:%02x:%02x:%02x\n", factory_MAC[0],factory_MAC[1],factory_MAC[2],factory_MAC[3],factory_MAC[4],factory_MAC[5]);
	//Delay(200);

    //printf("spi_flash_cache_enabled() returns %u\n", spi_flash_cache_enabled());

	init_deinit_TWDT();

	generate_random_seed();
	generate_random_seed();
	generate_random_seed();

	int res = nvs_flash_init();
	if(res!=ESP_OK)
	{
		for(int i=0;i<100;i++)
		{
			printf("nvs_flash_init(); returned %d\n",res);
			Delay(50);
		}
	}

	DAC_init();
	//DAC_test();

	/*
	//send 1 second of silence to the codec

	sample32 = 0;
	size_t written;
	for(unsigned long int i=0;i<I2S_AUDIOFREQ;i++)
	{
		i2s_write(I2S_NUM, (char *)&sample32, 4, &written, portMAX_DELAY);
	}
	*/

	/*
	//test sample playback

	flash_map_samples();
	if(samples_ptr1)
	{
		unsigned int adr;
		int16_t sample = 0;
		{
			for(adr = (unsigned int)samples_ptr1; (unsigned int)samples_ptr1 + SAMPLES_LENGTH; adr++)
			{
				((char*)&sample)[1] = ((char*)adr)[0];

				//one byte to each stereo channel
				i2s_write(I2S_NUM, (char *)&sample, 2, &written, portMAX_DELAY);
				i2s_write(I2S_NUM, (char *)&sample, 2, &written, portMAX_DELAY);

				//memcpy(&sample32, adr, 4);
				//i2s_write(I2S_NUM, (char *)&sample32, 4, &written, portMAX_DELAY);
			}
		}
	}
	*/

	LEDs_init(0);
	//LEDs_test();
	//while(1) { sleep(1); }

	encoders_init(1);
	light_sensors_init(1);

	#ifdef TOUCHPAD_TEST
	printf("main(): starting task [touch_pad_test]\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&touch_pad_test, "touch_pad_test_task", 4096, NULL, 10, NULL, 1);
	while(1);
	#else
	xTaskCreatePinnedToCore((TaskFunction_t)&touch_pad_scan, "touch_pad_scan_task", 4096, NULL, 10, NULL, 1);
	#endif //TOUCHPAD_TEST

	printf("init_echo_buffer();\n");
	init_echo_buffer();

	if(!self_tested())
	{
		#ifdef INIT_ENCODERS_SENSORS_FOR_SELF_TEST
		encoders_init(1);
		light_sensors_init(1);
		#endif
		run_self_test();
	}

	sine_waves_init();

	bytebeat_engine();

	/*
	//test encoders

	int leds_glowing[2] = {0,0};

	while(1)
	{
		if(encoder_results[ENCODER_LEFT]!=0)
		{
			LEDS_ALL_OFF;

			leds_glowing[0]+=encoder_results[ENCODER_LEFT];
			if(leds_glowing[0]<0)
			{
				leds_glowing[0] = 8;
			}
			if(leds_glowing[0]>8)
			{
				leds_glowing[0] = 1;
			}

			if(leds_glowing[0]>0)
			{
				set_led(leds_glowing[0]-1, 1);
			}

			//printf("~L:%d/%d\n",leds_glowing[0],leds_glowing[1]);

			encoder_results[ENCODER_LEFT] = 0;
		}
		if(encoder_results[ENCODER_RIGHT]!=0)
		{
			LEDS_ALL_OFF;

			leds_glowing[1]+=encoder_results[ENCODER_RIGHT];
			if(leds_glowing[1]<0)
			{
				leds_glowing[1] = 8;
			}
			if(leds_glowing[1]>8)
			{
				leds_glowing[1] = 1;
			}

			if(leds_glowing[1]>0)
			{
				set_led(leds_glowing[1]-1+8, 1);
			}

			//printf("~L:%d/%d\n",leds_glowing[0],leds_glowing[1]);

			encoder_results[ENCODER_RIGHT] = 0;
		}
		Delay(5);
	}
	*/

    printf("main(): finished\n");
}
