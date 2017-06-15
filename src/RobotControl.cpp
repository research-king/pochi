
#include "RobotControl.h"

#include "common.h"
#include "logprint.h"

#include "HTTPClient.h"

#include "TCPSocket.h"

#define WATSON_POST_URL "https://gateway-a.watsonplatform.net/visual-recognition/api/v3/classify?api_key=b59c9434aebd9ab825d1b015f31266475bbf9cd3&version=2016-05-20"
#define WATSON_GET_URL "https://gateway-a.watsonplatform.net/visual-recognition/api/v3/classify?api_key=b59c9434aebd9ab825d1b015f31266475bbf9cd3&url=http://weekly.ascii.jp/elem/000/000/346/346250/1032kanna-top_1200x.jpg&version=2016-05-19"

#define TEST_WIFI_SSID "S0512"
#define TEST_WIFI_PASS "0793353721"
#define TEST_TCP_IPADDR "172.20.10.4"
#define TEST_TCP_PORT 12345

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
        log("POCHI", LOGLEVEL_INFO, "WIFI STATION: %s RSSI: %hhd Ch: %hhd\r\n", ap[i].get_ssid(),
            ap[i].get_rssi(), ap[i].get_channel());
    }

    Thread::wait(1000);

    //-----------------------------------
    // AP接続
    //-----------------------------------
    while (1)
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
            break;
        }
        Thread::wait(5000);
    }

    log("POCHI", LOGLEVEL_MARK, "WIFI CONNECTED. IP=%s, RSSI=%d\r\n",
        m_pWifiInterface->get_ip_address(),
        m_pWifiInterface->get_rssi());

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
            sprintf(filename, "/" MOUNT_NAME "/image/img%d.jpg", filecnt++);
            if (m_pCameraControl->takeCamera(filename) == 1)
            {
                log("POCHI", LOGLEVEL_MARK, "CAMERA TAKE PICTURE SUCCESS. %s\r\n", filename);

#if 1
                //-----------------------------------
                // 撮影した画像をネット送信
                //-----------------------------------
                char buf[1024];
                FILE *fp = fopen(filename, "r");
                if (fp != NULL)
                {
                    fread(buf, 1, sizeof(buf), fp);
                    fclose(fp);
                }

                TCPSocket socket;
                socket.open(m_pWifiInterface);
                int ret = 0;
                const char* addr = TEST_TCP_IPADDR;
                int port = TEST_TCP_PORT;
                ret = socket.connect(addr, port);
                if (ret >= 0)
                {
                    log("POCHI", LOGLEVEL_INFO, "SOCKET CONNECT SUCCESS. ret=%d, addr=%s, port=%d\r\n", ret, addr, port);
                    ret = socket.send(buf, sizeof(buf));
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
                //GET data
                int ret = 0;
                char str[1024];
                HTTPClient http(m_pWifiInterface);
                HTTPMap map;

                printf("\nTrying to fetch page...\r\n");
                ret = http.get(WATSON_GET_URL, str, sizeof(str));
                if (!ret)
                {
                    printf("Page fetched successfully - read %d characters\r\n", strlen(str));
                    printf("Result: %s\n", str);
                }
                else
                {
                    printf("Error - ret = %d - HTTP return code = %d\r\n", ret, http.getHTTPResponseCode());
                }

                //POST data

                HTTPText inText(str, sizeof(str));
                map.put("Hello", "World");
                map.put("test", "1234");
                printf("\nTrying to post data...\r\n");
                ret = http.post(WATSON_POST_URL, map, &inText);
                if (!ret)
                {
                    printf("Executed POST successfully - read %d characters\r\n", strlen(str));
                    printf("Result: %s\n", str);
                }
                else
                {
                    printf("Error - ret = %d - HTTP return code = %d\r\n", ret, http.getHTTPResponseCode());
                }

                m_pWifiInterface->disconnect();
#endif
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
