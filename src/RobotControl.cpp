
#include "RobotControl.h"

#include "common.h"
#include "logprint.h"

#include "HTTPClient.h"

#include "TCPSocket.h"

#define TEST_WIFI_SSID "SSIDxxxx"
#define TEST_WIFI_PASS "PASSxxxx"
#define TEST_TCP_IPADDR "xxx.xxx.xxx.xxx"
#define TEST_TCP_PORT 12345

static char s_SendBuff[1024];
static char s_RecvBuff[1024];

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
 * WIFI接続
 * @param
 * @return
 */
bool RobotControl::isConnectWifi()
{
    if (m_pWifiInterface != NULL)
    {
        //if( m_pWifiInterface->isConnected() == true ) // TODO: 接続状態を判断するAPIなし？
        {
            return true;
        }
    }

    return false;
}

/**
 * WIFI接続
 * @param
 * @return
 */
bool RobotControl::connectWifi()
{
    if (isConnectWifi() == true)
    {
        return true;
    }

    if (m_pWifiInterface == NULL)
    {
        m_pWifiInterface = new ESP32Interface(WIFI_EN_PIN, WIFI_PI_PIN, WIFI_TX_PIN, WIFI_RX_PIN);
    }

    //-----------------------------------
    // AP SCAN
    //-----------------------------------
    WiFiAccessPoint ap[10]; /* Limit number of network arbitrary to 10 */

    int count = m_pWifiInterface->scan(ap, 10);
    for (int i = 0; i < count; i++)
    {
        log("POCHI", LOGLEVEL_INFO, "WIFI STATION: %s RSSI: %hhd\r\n", ap[i].get_ssid(),
            ap[i].get_rssi());
    }

    Thread::wait(1000);

    //-----------------------------------
    // AP接続
    //-----------------------------------
    while (true)
    {
        int ret = m_pWifiInterface->connect(TEST_WIFI_SSID, TEST_WIFI_PASS, NSAPI_SECURITY_WPA2);
        if (ret != 0)
        {
            log("POCHI", LOGLEVEL_ERROR, "WIFI CONNECT ERROR. SSID=%s, PASS=%s, ret=%d\r\n",
                TEST_WIFI_SSID,
                TEST_WIFI_PASS,
                ret);
        }
        else
        {
            log("POCHI", LOGLEVEL_MARK, "WIFI CONNECTED. ret=%d, IP=%s, RSSI=%d\r\n",
                ret,
                m_pWifiInterface->get_ip_address(),
                m_pWifiInterface->get_rssi());

            break;
        }

        Thread::wait(5000);
    }

    Thread::wait(1000);

    return true;
}

/**
 * main処理.
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::powerOn()
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

    //-----------------------------------
    // WIFI CONNECT
    //-----------------------------------
    connectWifi();

    m_isLife = 1;

    static int filecnt = 0;

    //-----------------------------------
    // メイン処理
    //-----------------------------------
    while (m_isLife)
    {
        //-----------------------------------
        // カメラ撮影
        //-----------------------------------
        if (m_pCameraControl != NULL)
        {
            char filename[32];
            sprintf(filename, "/" MOUNT_NAME "/img%d.jpg", filecnt++);
            if (m_pCameraControl->takeCamera(filename) == 1)
            {
                log("POCHI", LOGLEVEL_INFO, "CAMERA TAKE PICTURE SUCCESS. %s\r\n", filename);

                //-----------------------------------
                // 撮影した画像をネット送信
                //-----------------------------------
                FILE *fp = fopen(filename, "r");
                if (fp != NULL)
                {
                    fseek(fp, 0, SEEK_END);
                    int len = ftell(fp);
                    fseek(fp, 0, SEEK_SET); 

                    log("POCHI", LOGLEVEL_DEBUG, "IMAGE FILE SIZE = %d\r\n", len);

                    char *buf = new char[len];
                    fread(buf, 1, sizeof(buf), fp);

                    fclose(fp);

#if 0
                    TCPSocket socket;
                    socket.open(m_pWifiInterface);
                    int ret = 0;
                    const char *addr = TEST_TCP_IPADDR;
                    int port = TEST_TCP_PORT;
                    ret = socket.connect(addr, port);
                    if (ret >= 0)
                    {
                        log("POCHI", LOGLEVEL_INFO, "SOCKET CONNECT SUCCESS. ret=%d, addr=%s, port=%d\r\n", ret, addr, port);
                        ret = socket.send(buf, len);
                        if (ret > 0)
                        {
                            log("POCHI", LOGLEVEL_INFO, "SOCKET SEND SUCCESS. ret=%d\r\n", ret);
                        }
                        else
                        {
                            log("POCHI", LOGLEVEL_ERROR, "SOCKET SEND ERROR.\r\n");
                        }
                    }
                    else
                    {
                        log("POCHI", LOGLEVEL_ERROR, "SOCKET CONNECT ERROR. ret=%d, addr=%s, port=%d\r\n", ret, addr, port);
                    }

                    socket.close();

#else
                    int ret = 0;
                    HTTPClient http(m_pWifiInterface);

                    HTTPMap map;
                    map.put("Hello", "World");
                    map.put("test", "1234");

                    //POST data

                    HTTPText inText(s_RecvBuff, sizeof(s_RecvBuff));

                    const char *url = "www.google.com";
                    log("POCHI", LOGLEVEL_MARK, "HTTP POST.url=%s\r\n", url);

                    ret = http.get(url, &inText);
//                    ret = http.post(url, map, &inText);
                    if (!ret)
                    {
                        log("POCHI", LOGLEVEL_MARK, "HTTP POST SUCCESS. \r\n data=[%s] \r\n",
                            s_RecvBuff);
                    }
                    else
                    {
                        log("POCHI", LOGLEVEL_ERROR, "HTTP POST ERROR. ret=%d, rescode=%d\r\n",
                            ret,
                            http.getHTTPResponseCode());
                    }
#endif
                    delete buf;
                }
            }
            else
            {
                log("POCHI", LOGLEVEL_ERROR, "CAMERA TAKE PICTURE FAILED\r\n");
            }
        }

        Thread::wait(1000);
    }

    return true;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::gotoFront(void)
{
    if (m_pMotorControl == NULL)
    {
        return false;
    }

    m_pMotorControl->front();

    return true;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::gotoBack(void)
{
    if (m_pMotorControl == NULL)
    {
        return false;
    }

    m_pMotorControl->back();

    return true;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::turnLeft(void)
{
    if (m_pMotorControl == NULL)
    {
        return false;
    }

    m_pMotorControl->left();

    return true;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::turnRight(void)
{
    if (m_pMotorControl == NULL)
    {
        return false;
    }

    m_pMotorControl->right();

    return true;
}

/**
 * 
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::stop(void)
{
    if (m_pMotorControl == NULL)
    {
        return false;
    }

    m_pMotorControl->stop();

    return true;
}
