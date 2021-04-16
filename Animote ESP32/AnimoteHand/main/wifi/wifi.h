/*
 * wifi.h
 *
 *  Created on: Oct 23, 2020
 *      Author: tvr
 */

#ifndef MAIN_WIFI_WIFI_H_
#define MAIN_WIFI_WIFI_H_

#define HOST_SSID "Animote"
#define HOST_PASS "verybadmoves"

#define WEB_SERVER "192.168.1.1"
#define WEB_PORT 80
#define WEB_URL "192.168.1.1"

#define PREFIX "/Remote"


/*
 * COM SIGNALS
 */
#define BUTTON_PRESS_S		PREFIX "/buttonPress"
#define BUTTON_RELEASE_S	PREFIX "/buttonRelease"
#define IMU_RATE_S			PREFIX "/IMU_rate?"
#define IMU_DATA_S			PREFIX "/IMU"
#define STOP_S				PREFIX "/stop"

/*
 * COM ANSWERS
 */
#define COLOR_R		PREFIX "/color"
#define IMU_RATE_R "/IMU_rate"
#define IMU_RATE_R_len 9

void wifi_init(void);
int connectToHost();
int requestFromHost(char* request, char* responseBuf);

void formatIMUFrame(char* responseBuf, float heading, float pitch, float roll, float ax, float ay, float az);
int decodeIRResponse(char* response);

#endif /* MAIN_WIFI_WIFI_H_ */
