#include <mbed.h>

#include "MotorControl.h"
#include "CameraControl.h"

class RobotControl
{
public:
  RobotControl();

  unsigned char powerOn(void);

private:  
  unsigned char m_isLife;
  MotorControl* m_pMotorControl;
  CameraControl* m_pCameraControl;

  unsigned char think(void);

  unsigned char gotoFront(void);
  unsigned char gotoBack(void);
  unsigned char turnLeft(void);
  unsigned char turnRight(void);

  unsigned char stop(void);

  unsigned char playDance(void);
  unsigned char playSong(void);

};
