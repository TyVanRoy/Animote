/*
 * pinio.c
 *
 *  Created on: Oct 24, 2020
 *      Author: tvr
 */

#include "../pinio/pinio.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "driver/gpio.h"

#define GPIO_INPUT_PIN_SEL  ((1ULL<<PRIMARY_BUTTON_PIN) | (1ULL<<SECONDARY_BUTTON_PIN))

static void (*onButtonChange)(int primary, int high);

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    onButtonChange(gpio_num == PRIMARY_BUTTON_PIN, gpio_get_level(gpio_num));
}

void rgb_set(int value){
	gpio_set_level(R_GPIO, (value & 0b100) ? 1 : 0);
	gpio_set_level(G_GPIO, (value & 0b010) ? 1 : 0);
	gpio_set_level(B_GPIO, (value & 0b001) ? 1 : 0);
}

void ir_set(int value){
	gpio_set_level(IR2_GPIO, (value & 0b100) ? 1 : 0);
	gpio_set_level(IR1_GPIO, (value & 0b010) ? 1 : 0);
	gpio_set_level(IR0_GPIO, (value & 0b001) ? 1 : 0);
}

void init_leds(){
	// init IR LED
	gpio_pad_select_gpio(IR0_GPIO);
	gpio_set_direction(IR0_GPIO, GPIO_MODE_OUTPUT);

	gpio_pad_select_gpio(IR1_GPIO);
	gpio_set_direction(IR1_GPIO, GPIO_MODE_OUTPUT);

	gpio_pad_select_gpio(IR2_GPIO);
	gpio_set_direction(IR2_GPIO, GPIO_MODE_OUTPUT);

	// init RGB LED
	gpio_pad_select_gpio(R_GPIO);
	gpio_set_direction(R_GPIO, GPIO_MODE_OUTPUT);

	gpio_pad_select_gpio(G_GPIO);
	gpio_set_direction(G_GPIO, GPIO_MODE_OUTPUT);

	gpio_pad_select_gpio(B_GPIO);
	gpio_set_direction(B_GPIO, GPIO_MODE_OUTPUT);

	rgb_set(0b000);
	ir_set(0b000);
}

void init_buttons(void (*onButtonChangeCall)(int primary, int high)){

	onButtonChange = onButtonChangeCall;

	gpio_config_t io_conf;

	//interrupt of rising edge
	io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	gpio_set_intr_type(PRIMARY_BUTTON_PIN, GPIO_INTR_POSEDGE);
//	gpio_set_intr_type(SECONDARY_BUTTON_PIN, GPIO_INTR_ANYEDGE);

	//install gpio isr service
	gpio_install_isr_service(0);
	gpio_isr_handler_add(PRIMARY_BUTTON_PIN, gpio_isr_handler, (void*) PRIMARY_BUTTON_PIN);
	//gpio_isr_handler_add(SECONDARY_BUTTON_PIN, gpio_isr_handler, (void*) SECONDARY_BUTTON_PIN);
}
