
#include "RobotControl.h"

#include "common.h"
#include "logprint.h"

/**
 * コンストラクタ
 * @param
 * @return
 */
RobotControl::RobotControl()
{
    m_isLife = false;
}

/**
 * main処理.
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
unsigned char RobotControl::powerOn()
{
    log("POCHI", LOGLEVEL_MARK, "POWER ON\r\n");

    m_pMotorControl = new MotorControl(MOTOR_TX_PIN, MOTOR_RX_PIN);
    if (m_pMotorControl != NULL)
    {
        m_pMotorControl->init();
        log("POCHI", LOGLEVEL_MARK, "MOTOR OK\r\n");
    }

    m_pCameraControl = new CameraControl();
    if (m_pCameraControl != NULL)
    {
        m_pCameraControl->initCamera();
        log("POCHI", LOGLEVEL_MARK, "CAMERA OK\r\n");
    }

    m_isLife = 1;

    static int filecnt = 0;

    while (m_isLife)
    {
        // AI制御
        think();

        // カメラ撮影
        if (m_pCameraControl != NULL)
        {
            char filename[32];
            sprintf(filename, "/" MOUNT_NAME "/image/img%d.jpg", filecnt++);
            m_pCameraControl->takeCamera(filename);
            log("POCHI", LOGLEVEL_MARK, "CAMERA TAKE PICTURE\r\n");
        }

        Thread::wait(1000);
    }

    return 1;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
unsigned char RobotControl::think(void)
{
    return 1;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
unsigned char RobotControl::gotoFront(void)
{
    if (m_pMotorControl == NULL)
    {
        return 0;
    }

    m_pMotorControl->front();

    return 1;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
unsigned char RobotControl::gotoBack(void)
{
    if (m_pMotorControl == NULL)
    {
        return 0;
    }

    m_pMotorControl->back();

    return 1;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
unsigned char RobotControl::turnLeft(void)
{
    if (m_pMotorControl == NULL)
    {
        return 0;
    }

    m_pMotorControl->left();

    return 1;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
unsigned char RobotControl::turnRight(void)
{
    if (m_pMotorControl == NULL)
    {
        return 0;
    }

    m_pMotorControl->right();

    return 1;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
unsigned char RobotControl::stop(void)
{
    if (m_pMotorControl == NULL)
    {
        return 0;
    }

    m_pMotorControl->stop();

    return 1;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
unsigned char RobotControl::playDance(void)
{
    return 1;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
unsigned char RobotControl::playSong(void)
{
    return 1;
}
