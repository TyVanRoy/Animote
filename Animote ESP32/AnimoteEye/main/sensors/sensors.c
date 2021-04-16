#include "sensors.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/adc.h"

#include "../weird_access.h"

/* IR ADC */

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

static const adc_channel_t channel0 = ADC_CHANNEL_6;     //GPIO34
static const adc_channel_t channel1 = ADC_CHANNEL_7;     //GPIO35

static const adc_atten_t atten = ADC_ATTEN_DB_0;

static uint32_t readADC(adc_channel_t channel){
	uint32_t adc_reading = 0;
	//Multisampling
	for (int i = 0; i < NO_OF_SAMPLES; i++) {
		adc_reading += adc1_get_raw((adc1_channel_t)channel);
	}
	adc_reading /= NO_OF_SAMPLES;

	//Convert adc_reading to voltage in mV
	return raw_to_voltage(adc_reading);
}

void readIR(unsigned int* v0, unsigned int* v1){
	*v0 = readADC(channel0);
	*v1 = readADC(channel1);
}

static void initADC(){
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(channel0, atten);
	adc1_config_channel_atten(channel1, atten);
	finish_init_adc();
}

void ir_init(){
	initADC();
}


/* PIXY */

#define PIXY_UART UART_NUM_0
#define PIXY_SIG_LEN 6

#define BUF_SIZE (1024)

uint8_t pixySignal[PIXY_SIG_LEN];
uint8_t pixyBuffer[BUF_SIZE];

static void sendPixy(){
    uart_write_bytes(PIXY_UART, (const char *) pixySignal, PIXY_SIG_LEN);
}

static int readPixy(){
	return(uart_read_bytes(PIXY_UART, pixyBuffer, BUF_SIZE, 1 / portTICK_RATE_MS));
}

static void lampSignal(unsigned short on, uint8_t* data){
	data[2] = 22;
	data[3] = 2;
	data[4] = on;
	data[5] = 0;
}

static void blockSignal(uint8_t* data){
	data[2] = 32;
	data[3] = 2;
	data[4] = 1;
	data[5] = 3;
}

void setLamp(unsigned short on){
	lampSignal(on, pixySignal);
	sendPixy();
}

/*
 * UPDATE THIS TO USE THE COLOR_CODE
 */
static int getBlocks(uint8_t colorCode, uint8_t* blocks){
	blockSignal(pixySignal);
	sendPixy();
	int len = readPixy();
	if(len > 6){
		if(pixyBuffer[0] == 175 && pixyBuffer[1] == 193){
			// Copy block data
			for(int i = 0; i < 16; i++){
				blocks[i] = pixyBuffer[i];
			}

			// success
			return 0;
		}
		// debug
		return -2;
	}

	// error
	return -1;
}

int getBlock(unsigned short colorCode, unsigned short* x, unsigned short* y, unsigned short* w, unsigned short* h){
	uint8_t *blocks = (uint8_t*) malloc(20);

	int err = getBlocks(colorCode, blocks);
	if(!err){
		*x = (uint16_t)((((uint16_t)blocks[9]) << 8) | (blocks[8]));
		*y = (uint16_t)((((uint16_t)blocks[11]) << 8) | (blocks[10]));
		*w = (uint16_t)((((uint16_t)blocks[13]) << 8) | (blocks[12]));
		*h = (uint16_t)((((uint16_t)blocks[15]) << 8) | (blocks[14]));

		free(blocks);
		return err;
	}

	*x = -1;
	*y = -1;
	*w = -1;
	*h = -1;

	free(blocks);
	return err;
}

void pixy_init(){
	uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};
	uart_param_config(PIXY_UART, &uart_config);
	uart_driver_install(PIXY_UART, BUF_SIZE * 2, 0, 0, NULL, 0);

	// Signal constants
	pixySignal[0] = 0xae;
	pixySignal[1] = 0xc1;

	// Lamp animation
	setLamp(1);
	vTaskDelay(500 / portTICK_PERIOD_MS);
	setLamp(0);
	vTaskDelay(500 / portTICK_PERIOD_MS);
	setLamp(1);
	vTaskDelay(500 / portTICK_PERIOD_MS);
	setLamp(0);
}
