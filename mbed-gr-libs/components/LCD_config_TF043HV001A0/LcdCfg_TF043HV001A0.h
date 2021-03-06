
#ifndef LCD_CFG_TF043HV001A0_H
#define LCD_CFG_TF043HV001A0_H

#include "DisplayBace.h"

/* LCD Parameter */
#define LCD_INPUT_CLOCK                     (64.0)
#define LCD_OUTPUT_CLOCK                    (9)
#define LCD_PIXEL_WIDTH                     (480u)
#define LCD_PIXEL_HEIGHT                    (272u)
#define LCD_H_BACK_PORCH                    (43u)
#define LCD_H_FRONT_PORCH                   (8u)
#define LCD_H_SYNC_WIDTH                    (1u)
#define LCD_V_BACK_PORCH                    (12u)
#define LCD_V_FRONT_PORCH                   (4u)
#define LCD_V_SYNC_WIDTH                    (10u)

extern const DisplayBase::lcd_config_t LcdCfgTbl_TF043HV001A0;

#endif


