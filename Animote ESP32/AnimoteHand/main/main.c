#include <pinio/pinio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/dac.h"

#include "nvs_flash.h"
#include "esp_netif.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_task_wdt.h"

#include "pinio/pinio.h"
#include "wifi/wifi.h"
#include "orientation/orientation.h"
#include "driver/i2c.h"
#define I2C_MASTER_NUM I2C_NUM_0

#define PIXY_TEST		0

#define CONNECT_WAIT 	1000
#define BUTTON_WAIT 	250

#define DO_LOG			1

#define CONNECTING_COLOR	0b100
#define IDLE_COLOR 			0b001
#define ANIMATION_COLOR 	0b100

#define DEFAULT_IMU_SEND_RATE	500

uint8_t buttonPress = 0;

static void onButtonChange(int primary, int high){
	buttonPress = primary && high;
}

/*
 * Test Tasks
 */
static void ir_test(void *arg){

	ir_set(0b001);
	while(1){
		vTaskDelay(300 / portTICK_PERIOD_MS);

		esp_task_wdt_reset();
	}
}

static void rgb_test(void *arg){

	int i = 0;
	while(1) {
		vTaskDelay(500 / portTICK_PERIOD_MS);

		rgb_set(0b001);

		vTaskDelay(500 / portTICK_PERIOD_MS);

		rgb_set(0b010);

		vTaskDelay(500 / portTICK_PERIOD_MS);

		rgb_set(0b100);

		if(i % 500 == 0)
			esp_task_wdt_reset();
	}
}

static void button_test(void *arg){

	init_buttons(&onButtonChange);

	while(1){
		rgb_set(0b100);

		// Debounce
		vTaskDelay(100 / portTICK_PERIOD_MS);

		buttonPress = 0;

		while(!buttonPress){
			vTaskDelay(10 / portTICK_PERIOD_MS);
			esp_task_wdt_reset();
		}

		// Debounce
		vTaskDelay(100 / portTICK_PERIOD_MS);

		buttonPress = 0;

		rgb_set(0b001);

		while(!buttonPress){
			vTaskDelay(10 / portTICK_PERIOD_MS);
			esp_task_wdt_reset();
		}
	}

}

static void test_test(void *arg){
	while(1){
		printf("Hello, World!\n");
		vTaskDelay(500 / portTICK_PERIOD_MS);
		esp_task_wdt_reset();
	}
}

static void imu_test(void *arg){

	printf("IMU Ready\n");

	vector_t va, vg, vm;
	float heading, pitch, roll;
	int i = 0;
	int sendRate = 50;

	init_imu();

	init_buttons(&onButtonChange);

	while(1){

		rgb_set(0b100);

		// Debounce
		vTaskDelay(100 / portTICK_PERIOD_MS);

		buttonPress = 0;

		while(!buttonPress){
			vTaskDelay(10 / portTICK_PERIOD_MS);
			esp_task_wdt_reset();
		}

		// Debounce
		vTaskDelay(100 / portTICK_PERIOD_MS);

		i = 0;
		buttonPress = 0;

		rgb_set(0b001);

		printf("IMU Start\n");

		while (!buttonPress)
		{
			// Get the Accelerometer, Gyroscope and Magnetometer values.
			ESP_ERROR_CHECK(get_accel_gyro_mag(&va, &vg, &vm));

			// Transform these values to the orientation of our device.
			transform_accel_gyro(&va);
			transform_accel_gyro(&vg);
			transform_mag(&vm);

			// Apply the AHRS algorithm
			MadgwickAHRSupdate(DEG2RAD(vg.x), DEG2RAD(vg.y), DEG2RAD(vg.z),
			va.x, va.y, va.z,
			vm.x, vm.y, vm.z);


			if (i % sendRate == 0){
				/*
				 * IMU
				 */
				MadgwickGetEulerAnglesDegrees(&heading, &pitch, &roll);

				/*
				 * Send data
				 */
				printf("IMU:%f,%f,%f,%f,%f,%f\n", heading, pitch, roll, va.x, va.y, va.z);

				// Satisfy WDT
				esp_task_wdt_reset();
			}

			i++;
			pause();
		}
		printf("IMU Stop\n");
	}
}

/*
 * Tasks
 */

// 100 = 1 second
int sendRate = DEFAULT_IMU_SEND_RATE;	// iterations per send

static void idle_task(void *arg);

static void animation_task(void *arg)
{
    char* response = (char*) malloc(256);
    char* request = (char*) malloc(256);

#ifdef DO_LOG
	printf("\nAnimating... IMU rate = %d\n\n", sendRate);
#endif

	rgb_set(ANIMATION_COLOR);
	init_imu();

	vector_t va, vg, vm;
	float heading, pitch, roll;

	// Debounce
	vTaskDelay(300 / portTICK_PERIOD_MS);

	uint64_t i = 0;
	buttonPress = 0;
	while (!buttonPress)
	{

		// Get the Accelerometer, Gyroscope and Magnetometer values.
		ESP_ERROR_CHECK(get_accel_gyro_mag(&va, &vg, &vm));

		// Transform these values to the orientation of our device.
		transform_accel_gyro(&va);
		transform_accel_gyro(&vg);
		transform_mag(&vm);

		// Apply the AHRS algorithm
		MadgwickAHRSupdate(DEG2RAD(vg.x), DEG2RAD(vg.y), DEG2RAD(vg.z),
		va.x, va.y, va.z,
		vm.x, vm.y, vm.z);


		if (i % sendRate == 0){
			/*
			 * IMU
			 */
			MadgwickGetEulerAnglesDegrees(&heading, &pitch, &roll);

			/*
			 * Send data
			 */
			formatIMUFrame(request, heading, pitch, roll, va.x, va.y, va.z);
			requestFromHost(request, response);

//			rgb_set(decodeColorResponse(response));
			ir_set(decodeIRResponse(response));

			// Satisfy WDT
			esp_task_wdt_reset();
		}

		i++;
		pause();	// ?
	}

#ifdef DO_LOG
	printf("\nStoping...\n\n");
#endif

	requestFromHost(STOP_S, response);

	xTaskCreate(idle_task, "idle_task", 2048, NULL, 5, NULL);

	free(response);
	free(request);

	i2c_driver_delete(I2C_MASTER_NUM);
	vTaskDelete(NULL);
}

static void idle_task(void *arg)
{
#ifdef DO_LOG
	printf("\nIdling...\n\n");
#endif
	bool rateSet = false;

	rgb_set(IDLE_COLOR);

	// Debounce
	vTaskDelay(500 / portTICK_PERIOD_MS);

    char *response = (char*) malloc(64);
    uint8_t hardIdling = 1;
	buttonPress = 0;
	while(1){

		if(hardIdling && buttonPress){
			requestFromHost(BUTTON_PRESS_S, response);

			hardIdling = 0;
		}else if(!rateSet && !hardIdling){
			requestFromHost(IMU_RATE_S, response);
			if(strstr(response, IMU_RATE_R)){
				char* token = &response[IMU_RATE_R_len];
				sendRate = (int) atoi(token);
				rateSet = true;
			}
		}else if(!hardIdling && !gpio_get_level(PRIMARY_BUTTON_PIN)){
			requestFromHost(BUTTON_RELEASE_S, response);

			free(response);
			xTaskCreate(animation_task, "animation_task", 20000, NULL, 6, NULL);
			vTaskDelete(NULL);
			break;
		}

		vTaskDelay(BUTTON_WAIT / portTICK_PERIOD_MS);
		esp_task_wdt_reset();
	}
}

static void connect_task(void *arg){

#ifdef DO_LOG
	printf("\nConnecting...\n\n");
#endif

	rgb_set(CONNECTING_COLOR);

	while(connectToHost() != 0){
		vTaskDelay(CONNECT_WAIT / portTICK_PERIOD_MS);
		esp_task_wdt_reset();
	}

	// GPIO
	init_buttons(&onButtonChange);

	// Begin Idle task
	xTaskCreate(idle_task, "idle_task", 2048, NULL, 5, NULL);
	vTaskDelete(NULL);
}

static void calibrate_imu(void *arg){
	init_buttons(&onButtonChange);

	buttonPress = 0;
	while(!buttonPress){
		esp_task_wdt_reset();
		vTaskDelay(100 / portTICK_RATE_MS);
	}

	calibrate_gyro();

	buttonPress = 0;
	while(!buttonPress){
		esp_task_wdt_reset();
		vTaskDelay(100 / portTICK_RATE_MS);
	}

	calibrate_accel();

	buttonPress = 0;
	while(!buttonPress){
		esp_task_wdt_reset();
		vTaskDelay(100 / portTICK_RATE_MS);
	}

	calibrate_mag();


	vTaskDelay(100 / portTICK_RATE_MS);
	i2c_driver_delete(I2C_MASTER_NUM);

	vTaskDelete(NULL);
}

void app_main(void){

	init_leds();

	// WIFI
	ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
	wifi_init();


	xTaskCreate(connect_task, "connect_task", 2048, NULL, 5, NULL);
}
