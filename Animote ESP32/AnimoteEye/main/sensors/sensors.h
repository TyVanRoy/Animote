/*
 * sensors.h
 *
 *  Created on: Oct 23, 2020
 *      Author: tvr
 */

#ifndef MAIN_SENSORS_SENSORS_H_
#define MAIN_SENSORS_SENSORS_H_


void pixy_init();
void setLamp(unsigned short on);
int getBlock(unsigned short colorCode, unsigned short* x, unsigned short* y, unsigned short* w, unsigned short* h);

void ir_init();
void readIR(unsigned int* v0, unsigned int* v1);

#endif /* MAIN_SENSORS_SENSORS_H_ */
