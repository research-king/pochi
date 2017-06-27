#include <mbed.h>

#include "common.h"

#include "MotorControl.h"
#include "CameraControl.h"

#include "ESP32Interface.h"
#include "http_request.h"

class RobotControl
{
public:
  RobotControl();

  bool powerOn(void);

private:  
  unsigned char m_isLife;
  bool m_is_connect_wifi;
  CameraControl* m_pCameraControl;
  ESP32Interface* m_pWifiInterface;
  HttpRequest *m_HttpRequest;

  bool isConnectWifi(void);
  bool connectWifi(void);
  void onConnectWifiEvent(void);

  string postImageFileToServer(const char *url, const char *filename);
  string postMultiFileToServer(const char *url, const char *filename, const char *filename2);
};