/*
 * server.c
 *
 *  Created on: Oct 24, 2020
 *      Author: tvr
 */

#include "server.h"
#include "../sensors/sensors.h"
#include "../weird_access.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_vfs.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "esp_task_wdt.h"


#define CONFIG_AP_SSID		"Animote"
#define CONFIG_AP_PASS		"verybadmoves"
#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA_PSK

static const char *TAG = "server";

#define DO_LOG 1

#define PORT 3333

// Events
static EventGroupHandle_t event_group;
const int STA_CONNECTED_BIT = BIT0;

// Animation Attributes
bool unityReady = false;
bool ready = false;
bool go = false;
bool stop = false;
bool hasRate = false;
u8_t remoteColor = 1;
int IMURate;	// iterations per send
u8_t irSelect = 0;

int sock;
struct sockaddr_in6 source_addr;

QueueHandle_t xIMU_Q, xServerFrame_Q;

static void udp_server(void *pvParameters);

// AP event handler
static esp_err_t event_handler(void *ctx, system_event_t *event) {
    switch(event->event_id) {

    case SYSTEM_EVENT_AP_START:

		printf("* Wifi adapter started\n\n");

		init_mdns();
		printf("* mDNS service started\n");

		// start the UDP server task
		xTaskCreate(&udp_server, "udp_server", 20000, NULL, 5, NULL);
		printf("* UDP server started\n");

		break;

	case SYSTEM_EVENT_AP_STACONNECTED:

		xEventGroupSetBits(event_group, STA_CONNECTED_BIT);
		break;

	default:
        break;
    }

	return ESP_OK;
}


static void queue_init(){
	xIMU_Q = xQueueCreate(1, sizeof(IMUFrame));
	xServerFrame_Q = xQueueCreate(1, sizeof(ServerFrame));
}

static void parseAndStoreIMU(char* request){
	IMUFrame imu;

	char* token = strtok(&request[11], ",");
	imu.heading = (float) atof(token);

	token = strtok(NULL, ",");
	imu.pitch = (float) atof(token);

	token = strtok(NULL, ",");
	imu.roll = (float) atof(token);

	token = strtok(NULL, ",");
	imu.ax = (float) atof(token);

	token = strtok(NULL, ",");
	imu.ay = (float) atof(token);

	token = strtok(NULL, ",");
	imu.az = (float) atof(token);

	xQueueOverwrite(xIMU_Q, &imu);

#ifdef DO_LOG
	printf("\nIMU stored: %f, %f, %f, %f, %f, %f", imu.heading, imu.pitch, imu.roll, imu.ax, imu.ay, imu.az);
#endif
}

static void sensor_task(void *pvParameters){

	ServerFrame pir = {0, 0, 0, 0, 0, 0, 0};

	while(!stop){


		getBlock(0, &pir.x, &pir.y, &pir.w, &pir.h);
		pir.time = xTaskGetTickCount() / (portTICK_PERIOD_MS) * 100;

		xQueueOverwrite(xServerFrame_Q, &pir);

		vTaskDelay(10 / portTICK_PERIOD_MS);
	}

	ready = false;
	go = false;
	hasRate = false;

	vTaskDelete(NULL);
}

static void beginAnimation(){
	stop = false;

	queue_init();

	xTaskCreate(&sensor_task, "sensor_task", 2048, NULL, 4, NULL);
}

static int respond(char* response){
	int err = sendto(sock, response, strlen(response), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
	if (err < 0) {
		ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
		return -1;
	}
	return 0;
}

static void serveCurrentData(char* request, char* response){

	char* token = &request[CURRENT_Q_len];
	irSelect = (uint8_t) atoi(token);

	IMUFrame imu = {0, 0, 0};
	ServerFrame pir = {0, 0, 0, 0, 0, 0, 0};

	if(uxQueueMessagesWaiting(xIMU_Q) > 0){
		xQueueReceive(xIMU_Q, &imu, 0);
	}

	if(uxQueueMessagesWaiting(xServerFrame_Q) > 0){
		xQueueReceive(xServerFrame_Q, &pir, 0);
	}

	bzero(response, 256);
	sprintf(response, "/data_start:%d,%d,%d,%d,%d,%d,%f,%f,%f,%f,%f,%f,%lu", pir.x, pir.y, pir.w, pir.h, pir.irv0, pir.irv1, imu.heading, imu.pitch, imu.roll, imu.ax, imu.ay, imu.az, pir.time);
	respond(response);
}

static void serveUnity(char* request, char* response){
	static int currentServes = 0;

	if(stop){
#ifdef DO_LOG
		if(go)
			printf("\nSTOP --> Unity\n\n");
#endif
		respond(STOP_S);
		stop = false;
		return;
	}

	if(strstr(request, READY_Q)){
		if(!unityReady)
			unityReady = true;

		respond(ready ? READY_R : NOT_READY_R);
	}else if(strstr(request, IMU_RATE_S) && !hasRate){

		respond(THANKS);

		char* token = &request[IMU_RATE_len];
		IMURate = (int) atoi(token);
		hasRate = true;
	}else if(strstr(request, GO_Q)){

#ifdef DO_LOG
		if(go)
			printf("\nGO --> Unity\n\n");
#endif

		respond(go ? GO_R : NO_GO_R);
	}else if(strstr(request, CURRENT_Q)){

#ifdef DO_LOG
		if(go){
			printf("\nCURRENT --> Unity (%d)\n\n", currentServes++);
		}
#endif

		serveCurrentData(request, response);
	}else{
		respond(INVALID_REQ);
	}
}

/*
 * UPDATE THIS FOR COLOR CODE
 */
static void serveRemote(char* request, char* response){

	if(strstr(request, BUTTON_PRESS_S)){
		if(unityReady){
			ready = true;
			respond("ready");
		}else{
			respond("wait");
		}
	}else if(strstr(request, BUTTON_RELEASE_S)){

#ifdef DO_LOG
		if(go)
			printf("\nRemote --> GO\n\n");
#endif

		go = true;
		respond(THANKS);

		beginAnimation();


	}else if(strstr(request, IMU_RATE_Q)){
		if(hasRate){
			sprintf(response, IMU_RATE_S "%d", IMURate);
			respond(response);
		}else{
			respond("no rate yet");
		}
	}else if(strstr(request, IMU_DATA_S)){

		/* CHANGE THIS */

		sprintf(response, "%d", irSelect);
		respond(response);
		parseAndStoreIMU(request);
	}else if(strstr(request, STOP_S)){

#ifdef DO_LOG
		if(go)
			printf("\nRemote --> STOP\n\n");
#endif

		stop = true;
		respond(THANKS);
	}
	else{
		respond(INVALID_REQ);
	}
}

static void serve(char* request, char* response) {

	if(strstr(request, UNITY_PREFIX)){
		serveUnity(request, response);
	}else if(strstr(request, REMOTE_PREFIX)){
		serveRemote(request, response);
	}else{
		respond("Invalid Request!");
	}
}


// AP monitor task
void ap_monitor_task(void *pvParameter) {

	while(1) {
		xEventGroupWaitBits(event_group, STA_CONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		printf("New client connected\n");
	}
}

// HTTP server task
static void udp_server(void *pvParameters) {
	char rx_buffer[128];
	char addr_str[128];
	int addr_family;
	int ip_protocol;

	char response[256];

	int requests = 0;

	while (1) {

		struct sockaddr_in dest_addr;
		dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(PORT);
		addr_family = AF_INET;
		ip_protocol = IPPROTO_IP;
		inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

		/* */

		sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
		if (sock < 0) {
			ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
			break;
		}
		ESP_LOGI(TAG, "Socket created");

		int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if (err < 0) {
			ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
		}
		ESP_LOGI(TAG, "Socket bound, port %d", PORT);

		while (1) {

			ESP_LOGI(TAG, "Waiting for data");
			socklen_t socklen = sizeof(source_addr);
			int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

			// Error occurred during receiving
			if (len < 0) {
				ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
				break;
			}
			// Data received
			else {
				// Get the sender's ip address as string
				if (source_addr.sin6_family == PF_INET) {
					inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
				} else if (source_addr.sin6_family == PF_INET6) {
					inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
				}

				rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
				ESP_LOGI(TAG, "(%d) Received %d bytes from %s:", requests++, len, addr_str);
				ESP_LOGI(TAG, "%s", rx_buffer);

				serve(rx_buffer, response);
			}
		}

		if (sock != -1) {
			ESP_LOGE(TAG, "Shutting down socket and restarting...");
			shutdown(sock, 0);
			close(sock);
		}
	}
	vTaskDelete(NULL);
}

void ap_server_wifi_init() {
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

	printf("ESP32 SoftAP UDP Server\n\n");

	/* */

	// create the event group to handle wifi events
	event_group = xEventGroupCreate();

	tcpip_adapter_init();
	printf("TCP adapter initialized\n");

	// stop DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	printf("- DHCP server stopped\n");

	// assign a static IP to the network interface
	tcpip_adapter_ip_info_t info;
	memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 1, 1);
	IP4_ADDR(&info.gw, 192, 168, 1, 1);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
	printf("TCP adapter configured with IP 192.168.1.1\n");

	// start the DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	printf("DHCP server started\n");

	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	printf("Event loop initialized\n");

	// initialize the wifi stack in AccessPoint mode with config in RAM
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	printf("Wifi adapter configured in SoftAP mode\n");

	// configure the wifi connection and start the interface
	wifi_config_t ap_config = {
        .ap = {
            .ssid = CONFIG_AP_SSID,
            .password = CONFIG_AP_PASS,
			.ssid_len = 7,
			.channel = 7,
			.authmode = CONFIG_AP_AUTHMODE,
			.ssid_hidden = false,
			.max_connection = 4,
			.beacon_interval = 20,
        },
    };
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	printf("Wifi network settings applied\n");


	// start the wifi interface
	ESP_ERROR_CHECK(esp_wifi_start());
	printf("Wifi adapter starting...\n");

	#ifdef DO_LOG
		printf("\nDid initialize\n\n");
	#endif

	xTaskCreate(&ap_monitor_task, "ap_monitor_task", 1024, NULL, 5, NULL);
}
