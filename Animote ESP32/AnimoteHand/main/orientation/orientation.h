/*
 * orientation.h
 *
 *  Created on: Oct 24, 2020
 *      Author: tvr
 */

#ifndef MAIN_ORIENTATION_ORIENTATION_H_
#define MAIN_ORIENTATION_ORIENTATION_H_

#include "../../components/ahrs/MadgwickAHRS.h"
#include "../../components/mpu9250/mpu9250.h"
#include "../../components/mpu9250/calibrate.h"
#include "../../components/mpu9250/common.h"

void init_imu(void);

void transform_accel_gyro(vector_t *v);
void transform_mag(vector_t *v);

#endif /* MAIN_ORIENTATION_ORIENTATION_H_ */
