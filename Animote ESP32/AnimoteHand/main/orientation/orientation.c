/*
 * orientation.c
 *
 *  Created on: Oct 24, 2020
 *      Author: tvr
 */


#include "orientation.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


calibration_t cal = {
	//--- Original Cal Data

	.mag_offset = {.x = 25.183594, .y = 57.519531, .z = -62.648438},
	.mag_scale = {.x = 1.513449, .y = 1.557811, .z = 1.434039},

	.accel_offset = {.x = 0.020900, .y = 0.014688, .z = -0.002580},
	.accel_scale_lo = {.x = -0.992052, .y = -0.990010, .z = -1.011147},
	.accel_scale_hi = {.x = 1.013558, .y = 1.011903, .z = 1.019645},

	.gyro_bias_offset = {.x = 0.303956, .y = -1.049768, .z = -0.403782}


	//--- New Cal Data

//	.mag_offset = {.x = -41.507812, .y = 77.853516, .z = -14.015625},
//	.mag_scale = {.x = 1.727287, .y = 1.379800, .z = 1.436132},

	//.accel_offset = {.x = 0.024564, .y = -0.003053, .z = -0.022465},
	//.accel_scale_lo = {.x = 1.003983, .y = 1.001911, .z = 0.996944},
	//.accel_scale_hi = {.x = -0.996185, .y = -0.997676, .z = -1.021887},

//    .gyro_bias_offset = {.x = 2.262032, .y = -1.178946, .z = -0.096606}
};

/**
 * Transformation:
 *  - Rotate around Z axis 180 degrees
 *  - Rotate around X axis -90 degrees
 * @param  {object} s {x,y,z} sensor
 * @return {object}   {x,y,z} transformed
 */
void transform_accel_gyro(vector_t *v)
{
  float x = v->x;
  float y = v->y;
  float z = v->z;

  v->x = -x;
  v->y = -z;
  v->z = -y;
}

/**
 * Transformation: to get magnetometer aligned
 * @param  {object} s {x,y,z} sensor
 * @return {object}   {x,y,z} transformed
 */
void transform_mag(vector_t *v)
{
  float x = v->x;
  float y = v->y;
  float z = v->z;

  v->x = -y;
  v->y = z;
  v->z = -x;
}

void init_imu(void){
	 i2c_mpu9250_init(&cal);
	 MadgwickAHRSinit(SAMPLE_FREQ_Hz, 0.8);
}

