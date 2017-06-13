
#include <mbed.h>

#include "common.h"

#include "EasyAttach_CameraAndLCD.h"
#include "JPEG_Converter.h"

class CameraControl
{
private:
  /* jpeg convert */
   JPEG_Converter m_JpegConverter;

   DisplayBase    m_Display;

public:
  CameraControl();

  unsigned char initCamera();
  unsigned char takeCamera(const char *filename);
};
