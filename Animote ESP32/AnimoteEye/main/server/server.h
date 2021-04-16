/*
 * server.h
 *
 *  Created on: Oct 23, 2020
 *      Author: tvr
 */

#ifndef MAIN_SERVER_SERVER_H_
#define MAIN_SERVER_SERVER_H_


#define UNITY_PREFIX		"/Unity"
#define REMOTE_PREFIX		"/Remote"

/*
 * REMOTE COMS
 */
#define BUTTON_PRESS_S		"/buttonPress"
#define BUTTON_RELEASE_S	"/buttonRelease"
#define IMU_DATA_S			"/IMU"
#define IMU_RATE_Q			"/IMU_rate?"
#define STOP_S		"/stop"

#define THANKS		"Thanks!"
#define THANKS_len	7

#define INVALID_REQ		"Invalid request!"
#define INVALID_REQ_len 16


/*
 * UNITY COMS
 */
#define CURRENT_Q	"/current?"
#define CURRENT_Q_len 9
#define READY_Q		"/ready?"
#define IMU_RATE_S  "/IMU_rate"
#define IMU_RATE_len 15
#define GO_Q		"/go?"

/*
 * COM RESPONSES
 */
#define READY_R		"ready_confirm"
#define GO_R		"go_confirm"

#define NOT_READY_R	"ready_decline"
#define NO_GO_R		"go_decline"

#define COLOR_R		"/color"

typedef struct KeyFrame{
	unsigned int isValid;
	unsigned int x, y, w, h;
	unsigned int irv0, irv1;
	float heading, pitch, roll;
	long time;
} KeyFrame;

typedef struct IMUFrame{
	float heading;
	float pitch;
	float roll;
	float ax;
	float ay;
	float az;
} IMUFrame;

typedef struct ServerFrame{
	unsigned short x, y, w, h;
	unsigned int irv0, irv1;
	unsigned long time;
} ServerFrame;

void ap_server_wifi_init();


#endif /* MAIN_SERVER_SERVER_H_ */
