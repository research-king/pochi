#ifndef COMMON_H
#define COMMON_H

#define BOARDTYPE_PEACH 1
#define BOARDTYPE_LYCHEE 2

#define BOARDTYPE   BOARDTYPE_LYCHEE

#define MOUNT_NAME "storage"

// ---- LED ---- 
#define HEARTBEAT_LED       LED1
#define NETWORK_LED         LED2
#define CAMERA_LED          LED3

// ---- MOTOR ---- 
#define MOTOR_TX_PIN        P5_15
#define MOTOR_RX_PIN        P5_14

// ---- WIFI  ----
#define WIFI_EN_PIN         P5_3
#define WIFI_PI_PIN         P3_14
#define WIFI_TX_PIN         P3_15
#define WIFI_RX_PIN         P0_2

#endif  //COMMON_H
