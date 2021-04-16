#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "nvs_flash.h"
#include "mdns.h"
#include "esp_adc_cal.h"

#include "server/server.h"
#include "sensors/sensors.h"

static esp_adc_cal_characteristics_t *adc_chars;

// TESTS
static void testIR(void *pvParameters){
	ir_init();

	uint32_t v1, v2;
	while(1){

		readIR(&v1, &v2);
        printf("Unit 0 Voltage: %dmV\tUnit 1 Voltage: %dmV\n\n", v1, v2);

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

// Real stuff

void init_mdns(void){
	ESP_ERROR_CHECK(mdns_init());
}

void finish_init_adc(void){
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 1100, adc_chars);
}

unsigned int raw_to_voltage(unsigned int adc_reading){
	return esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
}

static void init_sequence(void *pvParameters) {

	// Wait for the Pixy power on sequence.
	vTaskDelay(2000 / portTICK_PERIOD_MS);

	pixy_init();
//	ir_init();

	ESP_ERROR_CHECK(nvs_flash_init());
	printf("- NVS initialized\n");
	ESP_ERROR_CHECK(esp_netif_init());

	// Initialize wifi, initialize AP, start server
	ap_server_wifi_init();

	vTaskDelete(NULL);
}

void app_main(void){
	xTaskCreate(init_sequence, "init_sequence", 2048, NULL, 5, NULL);
//	xTaskCreate(testIR, "testIR", 2048, NULL, 5, NULL);
}
