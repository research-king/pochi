#include "CameraControl.h"

#include "dcache-control.h"

#include "common.h"

#define CAMERA_LOG_ENABLE 0

#if CAMERA_LOG_ENABLE
#define DEBUG_PRINT(x) printf(x)
#else
#define DEBUG_PRINT(x)
#endif

/* Video input and LCD layer 0 output */
#define VIDEO_FORMAT (DisplayBase::VIDEO_FORMAT_YCBCR422)
#define GRAPHICS_FORMAT (DisplayBase::GRAPHICS_FORMAT_YCBCR422)
#define WR_RD_WRSWA (DisplayBase::WR_RD_WRSWA_32_16BIT)
#define DATA_SIZE_PER_PIC (2u)

/*! Frame buffer stride: Frame buffer stride should be set to a multiple of 32 or 128
    in accordance with the frame buffer burst transfer mode. */
#define VIDEO_PIXEL_HW (640u) /* VGA */
#define VIDEO_PIXEL_VW (480u) /* VGA */

#define FRAME_BUFFER_STRIDE (((VIDEO_PIXEL_HW * DATA_SIZE_PER_PIC) + 31u) & ~31u)
#define FRAME_BUFFER_HEIGHT (VIDEO_PIXEL_VW)

#if defined(__ICCARM__)
#pragma data_alignment = 32
static uint8_t user_frame_buffer0[FRAME_BUFFER_STRIDE * FRAME_BUFFER_HEIGHT] @".mirrorram";
#pragma data_alignment = 4
#else
static uint8_t user_frame_buffer0[FRAME_BUFFER_STRIDE * FRAME_BUFFER_HEIGHT] __attribute((section("NC_BSS"), aligned(32)));
#endif

#if defined(__ICCARM__)
#pragma data_alignment = 32
static uint8_t JpegBuffer[1024 * 63];
#else
static uint8_t JpegBuffer[1024 * 63] __attribute((aligned(32)));
#endif

/**
 * コンストラクタ
 * @param
 * @return
 */
CameraControl::CameraControl()
{
    DEBUG_PRINT("CameraControl:CameraControl()\r\n");
}

/**
 * 初期化処理.
 * @param
 * @return TRUE(1):成功, FALSE(0):失敗
 */
unsigned char CameraControl::initCamera()
{
    DEBUG_PRINT("CameraControl:initCamera()\r\n");

    // Initialize the background to black
    for (uint32_t i = 0; i < sizeof(user_frame_buffer0); i += 2)
    {
        user_frame_buffer0[i + 0] = 0x10;
        user_frame_buffer0[i + 1] = 0x80;
    }

    // Camera
    EasyAttach_Init(m_Display);

    // Video capture setting (progressive form fixed)
    m_Display.Video_Write_Setting(
        DisplayBase::VIDEO_INPUT_CHANNEL_0,
        DisplayBase::COL_SYS_NTSC_358,
        (void *)user_frame_buffer0,
        FRAME_BUFFER_STRIDE,
        VIDEO_FORMAT,
        WR_RD_WRSWA,
        VIDEO_PIXEL_VW,
        VIDEO_PIXEL_HW);

    EasyAttach_CameraStart(m_Display, DisplayBase::VIDEO_INPUT_CHANNEL_0);

    return 1;
}

/**
 * 撮影処理
 * @param
 * @return TRUE(1):成功, FALSE(0):失敗
 */
unsigned char CameraControl::takeCamera(const char *filename)
{
    DEBUG_PRINT("CameraControl:takeCamera()\r\n");

    size_t jcu_encode_size;
    JPEG_Converter::bitmap_buff_info_t bitmap_buff_info;
    JPEG_Converter::encode_options_t encode_options;

    bitmap_buff_info.width = VIDEO_PIXEL_HW;
    bitmap_buff_info.height = VIDEO_PIXEL_VW;
    bitmap_buff_info.format = JPEG_Converter::WR_RD_YCbCr422;
    bitmap_buff_info.buffer_address = (void *)user_frame_buffer0;

    encode_options.encode_buff_size = sizeof(JpegBuffer);
    encode_options.p_EncodeCallBackFunc = NULL;
    encode_options.input_swapsetting = JPEG_Converter::WR_RD_WRSWA_32_16_8BIT;

    jcu_encode_size = 0;
    dcache_invalid(JpegBuffer, sizeof(JpegBuffer));
    if (m_JpegConverter.encode(&bitmap_buff_info, JpegBuffer, &jcu_encode_size, &encode_options) != JPEG_Converter::JPEG_CONV_OK)
    {
        jcu_encode_size = 0;
    }

    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        DEBUG_PRINT("CameraControl:takeCamera FAILED\r\n");
        return 0;
    }
    else
    {
        fwrite(JpegBuffer, sizeof(char), (int)jcu_encode_size, fp);
        fclose(fp);
    }

    DEBUG_PRINT("CameraControl:takeCamera SUCCESS\r\n");
    return 1;
}
