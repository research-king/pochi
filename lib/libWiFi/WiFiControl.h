#include <mbed.h>

class WiFiControl
{
private:

public:
  WiFiControl(PinName tx, PinName rx);

  unsigned char connect();
  unsigned char disconnect();
};
