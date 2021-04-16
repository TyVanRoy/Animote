#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include <string.h>

#include "wifi.h"


const int CONNECTED_BIT = BIT0;

#define HOST_IP_ADDR "192.168.1.1"

#define PORT 3333

static EventGroupHandle_t wifi_event_group;

static const char *TAG = "remote_wifi";

char rx_buffer[128];
char addr_str[128];
int addr_family;
int ip_protocol;

struct sockaddr_in dest_addr;
int sock;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void formatIMUFrame(char* responseBuf, float heading, float pitch, float roll, float ax, float ay, float az){
	sprintf(responseBuf, IMU_DATA_S "%f,%f,%f,%f,%f,%f", heading, pitch, roll, ax, ay, az);
}

int decodeIRResponse(char* response){
	return (int) atoi(response);
}

static struct addrinfo *res;

void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = HOST_SSID,
            .password = HOST_PASS,
			.channel = 7
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

// Attempts to connect to host in a loop until success
int connectToHost(void){

	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
						false, true, portMAX_DELAY);
	ESP_LOGI(TAG, "Connected to AP");


	while(1){
		dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(PORT);
		addr_family = AF_INET;
		ip_protocol = IPPROTO_IP;
		inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

		sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
		if (sock < 0) {
			ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
			continue;
		}
		ESP_LOGI(TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);
		break;
	}


	return 0;
}

int requestFromHost(char* request, char* responseBuf){
	int err = sendto(sock, request, strlen(request), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (err < 0) {
		ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
	}else{
		ESP_LOGI(TAG, "Message sent");

		struct sockaddr_in source_addr; // Large enough for both IPv4 or IPv6
		socklen_t socklen = sizeof(source_addr);
		int len = recvfrom(sock, responseBuf, 127, 0, (struct sockaddr *)&source_addr, &socklen);

		// Error occurred during receiving
		if (len < 0) {
			ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
		}
		// Data received
		else {
			responseBuf[len] = 0; // Null-terminate whatever we received and treat like a string
			ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
			ESP_LOGI(TAG, "%s", responseBuf);

			return 0;
		}
	}

	if (sock != -1) {
		ESP_LOGE(TAG, "Shutting down socket and restarting...");
		shutdown(sock, 0);
		close(sock);

		return -1;
	}

	return 0;
}
