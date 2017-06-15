#include <mbed.h>

#include "common.h"

#include "MotorControl.h"
#include "CameraControl.h"

#include "ESP32Interface.h"

class RobotControl
{
public:
  RobotControl();

  bool powerOn(void);

private:  
  unsigned char m_isLife;
  MotorControl* m_pMotorControl;
  CameraControl* m_pCameraControl;
  ESP32Interface* m_pWifiInterface;

  bool isConnectWifi(void);
  bool connectWifi(void);
  void onConnectWifiEvent(void);

  bool gotoFront(void);
  bool gotoBack(void);
  bool turnLeft(void);
  bool turnRight(void);

  bool stop(void);

  bool playDance(void);
  bool playSong(void);

};
