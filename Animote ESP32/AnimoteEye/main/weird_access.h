/*
 * weird_access.h
 *
 *  Created on: Oct 26, 2020
 *      Author: tvr
 */

#ifndef MAIN_WEIRD_ACCESS_H_
#define MAIN_WEIRD_ACCESS_H_


void init_mdns(void);

void finish_init_adc(void);

unsigned int raw_to_voltage(unsigned int adc_reading);


#endif /* MAIN_WEIRD_ACCESS_H_ */
