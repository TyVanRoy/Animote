/*
 * pinio.h
 *
 *  Created on: Oct 24, 2020
 *      Author: tvr
 */

#ifndef MAIN_PINIO_PINIO_H_
#define MAIN_PINIO_PINIO_H_

// LED GPIO
#define IR0_GPIO 17
#define IR1_GPIO 16
#define IR2_GPIO 1

#define R_GPIO 19
#define G_GPIO 18
#define B_GPIO 5

// Button GPIO
#define PRIMARY_BUTTON_PIN 36
#define SECONDARY_BUTTON_PIN 39

void init_leds();
void init_buttons(void (*onButtonChangeCall)(int primary, int high));
void rgb_set(int value);
void ir_set(int value);


#endif /* MAIN_PINIO_PINIO_H_ */
