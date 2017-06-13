#include <mbed.h>

class MotorControl
{
private:
  RawSerial ser;

public:
  MotorControl(PinName tx, PinName rx);

  unsigned char init();
  unsigned char front();
  unsigned char back();
  unsigned char left();
  unsigned char right();
  unsigned char stop();
};
