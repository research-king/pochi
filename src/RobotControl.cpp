
#include "RobotControl.h"

#include "common.h"
#include "logprint.h"

#include "TCPSocket.h"

#define WATSON_POST_URL "http://gateway-a.watsonplatform.net/visual-recognition/api/v3/classify?api_key=c3659879198b8ccb65af7464b051a13a08567fb0&version=2016-05-2"
#define RKING_POST_URL "http://104.199.222.173/r-king/watson/webapi.php"

#define TEST_WIFI_SSID "S0512"
#define TEST_WIFI_PASS "0793353721"

//#define TEST_WIFI_SSID "4CE67630E22B"
//#define TEST_WIFI_PASS "t3340pn5mkmkh"

static char g_ssid[PATH_MAX];
static char g_pass[PATH_MAX];
static char g_post_url[PATH_MAX];

bool is_connect_wifi = false;

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
    if (is_connect_wifi == true)
    {
        return true;
    }

    if (m_pWifiInterface == NULL)
    {
        m_pWifiInterface = new ESP32Interface(WIFI_EN_PIN, WIFI_PI_PIN, WIFI_TX_PIN, WIFI_RX_PIN);
    }

    //-----------------------------------
    // AP接続
    //-----------------------------------
    memset(g_ssid, 0, sizeof(g_ssid));
    memset(g_pass, 0, sizeof(g_pass));
    memset(g_post_url, 0, sizeof(g_post_url));

    FILE *fp = fopen("/" MOUNT_NAME "/lychee_config.txt", "r");
    if (fp != NULL)
    {
        fgets(g_ssid, PATH_MAX, fp);
        fgets(g_pass, PATH_MAX, fp);
        fgets(g_post_url, PATH_MAX, fp);
        fclose(fp);
        if (g_ssid[strlen(g_ssid) - 2] == '\r')
        {
            // CR-LF
            g_ssid[strlen(g_ssid) - 2] = '\0';
            g_pass[strlen(g_pass) - 2] = '\0';
            g_post_url[strlen(g_post_url) - 2] = '\0';
        }
        else
        {
            // LF
            g_ssid[strlen(g_ssid) - 1] = '\0';
            g_pass[strlen(g_pass) - 1] = '\0';
            g_post_url[strlen(g_post_url) - 1] = '\0';
        }
    }
    else
    {
        strcpy(g_ssid, TEST_WIFI_SSID);
        strcpy(g_pass, TEST_WIFI_PASS);
        strcpy(g_post_url, WATSON_POST_URL);
    }

    while (1)
    {
        log("POCHI", LOGLEVEL_INFO, "WIFI CONNECTING... SSID=%s, PASS=%s\r\n",
            g_ssid,
            g_pass);

        int ret = 0;

        // APサーチ開始
        WiFiAccessPoint ap[10]; /* Limit number of network arbitrary to 10 */

        int count = m_pWifiInterface->scan(ap, 10);
        int existap = -1;
        for (int i = 0; i < count; i++)
        {
            if (strcmp(ap[i].get_ssid(), g_ssid) == 0)
            {
                log("POCHI", LOGLEVEL_MARK, "WIFI STATION: %s RSSI: %hhd\r\n", ap[i].get_ssid(),
                    ap[i].get_rssi());

                existap = i;
            }
            else
            {
                log("POCHI", LOGLEVEL_INFO, "WIFI STATION: %s RSSI: %hhd\r\n", ap[i].get_ssid(),
                    ap[i].get_rssi());
            }
        }

        // APサーチ成功
        if (existap >= 0)
        {
            log("POCHI", LOGLEVEL_MARK, "WIFI AP FIND SUCCESS. SSID=%s, PASS=%s\r\n",
                g_ssid,
                g_pass);
        }
        else
        {
            log("POCHI", LOGLEVEL_WARN, "WIFI AP NOT FOUND. SSID=%s, PASS=%s\r\n",
                g_ssid,
                g_pass);            
        }

        ret = m_pWifiInterface->connect(g_ssid, g_pass, NSAPI_SECURITY_WPA2);

        if (ret != 0)
        {
            log("POCHI", LOGLEVEL_ERROR, "WIFI CONNECT ERROR. SSID=%s, PASS=%s, ret=%d\r\n",
                g_ssid,
                g_pass,
                ret);
        }
        else
        {
            break;
        }

        Thread::wait(1000);
    }

    log("POCHI", LOGLEVEL_MARK, "WIFI CONNECTED. IP=%s, RSSI=%d\r\n",
        m_pWifiInterface->get_ip_address(),
        m_pWifiInterface->get_rssi());

    is_connect_wifi = true;

    return true;
}

/**
 * ファイルポスト処理
 * @param
 * @return
 */
string RobotControl::postImageFileToServer(const char *url, const char *filename)
{
    string resultstr = "";

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

        // create post header
        char s_boundary[] = "------WebKitFormBoundaryZRUHAuSd1pDiYfK5\r\n";
        char s_contdisp[] = "Content-Disposition: form-data; name=\"image\"; filename=\"cam_000.jpg\"\r\n";
        char s_conttype[] = "Content-Type: image/jpeg\r\n\r\n";
        char e_boundary[] = "\r\n------WebKitFormBoundaryZRUHAuSd1pDiYfK5--";

        char *reqbuf = new char[len + strlen(s_boundary) + strlen(s_contdisp) + strlen(s_conttype) + strlen(e_boundary)];
        char *imgbuf = new char[len];

        memcpy(reqbuf, s_boundary, strlen(s_boundary));
        memcpy(&reqbuf[strlen(s_boundary)], s_contdisp, strlen(s_contdisp));
        memcpy(&reqbuf[strlen(s_boundary) + strlen(s_contdisp)], s_conttype, strlen(s_conttype));
        fread(imgbuf, 1, len, fp);
        fclose(fp);
        memcpy(&reqbuf[strlen(s_boundary) + strlen(s_contdisp) + strlen(s_conttype)], imgbuf, len);
        memcpy(&reqbuf[strlen(s_boundary) + strlen(s_contdisp) + strlen(s_conttype) + len], e_boundary, strlen(e_boundary));

        // make http request

        log("POCHI", LOGLEVEL_MARK, "SEND POST. url=%s\r\n", url);

        m_HttpRequest = new HttpRequest(m_pWifiInterface, HTTP_POST, url);

        m_HttpRequest->set_header("Content-Type", "multipart/form-data; boundary=----WebKitFormBoundaryZRUHAuSd1pDiYfK5");
        m_HttpRequest->set_header("Connection", "keep-alive");
        m_HttpRequest->set_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.99 Safari/537.36");
        HttpResponse *response;

        // send request
        response = m_HttpRequest->send(reqbuf, strlen(s_boundary) + strlen(s_contdisp) + strlen(s_conttype) + len + strlen(e_boundary));
        log("POCHI", LOGLEVEL_INFO, "POST FILE SIZE = %d\r\n", strlen(s_boundary) + strlen(s_contdisp) + strlen(s_conttype) + len + strlen(e_boundary));

        delete[] reqbuf;
        delete[] imgbuf;

        if (response != NULL)
        {
            log("POCHI", LOGLEVEL_INFO, "RESPONSE status is %d - %s\r\n", response->get_status_code(), response->get_status_message().c_str());
            log("POCHI", LOGLEVEL_INFO, "RESPONSE body is:\r\n[len=%d][%s]\r\n",
                response->get_body_as_string().size(),
                response->get_body_as_string().c_str());

            resultstr += response->get_body_as_string();
        }
        else
        {
            log("POCHI", LOGLEVEL_WARN, "RESPONSE IS NULL!\r\n");
        }

        delete response;
        delete m_HttpRequest; // also clears out the response        
    }
    else
    {
        log("POCHI", LOGLEVEL_ERROR, "SEND FILE NOT FOUND!\r\n");
    }

    return resultstr;
}

/**
 * 複数ファイルポスト処理
 * @param
 * @return
 */
string RobotControl::postMultiFileToServer(const char *url, const char *filename, const char* filename2)
{
    string resultstr = "";

    // TODO:

    return resultstr;
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
            char filename[PATH_MAX];
            sprintf(filename, "/" MOUNT_NAME "/img%d.jpg", filecnt++);
            if (m_pCameraControl->takeCamera(filename) == 1)
            {
                log("POCHI", LOGLEVEL_INFO, "CAMERA TAKE PICTURE SUCCESS. %s\r\n", filename);

                string response;

                response = postImageFileToServer((char *)RKING_POST_URL, filename);
                if (response.size() == 0 )
                {
                    log("POCHI", LOGLEVEL_ERROR, "HTTP POST FAILED! %d %s\r\n", response.size(), response.c_str());
                }
                else
                {
                    log("POCHI", LOGLEVEL_MARK, "HTTP POST SUCCESS!. reslen=%d\r\n", 
                        response.size());
                }

                Thread::wait(5000);

                response = postImageFileToServer((char *)WATSON_POST_URL, filename);
                if (response.size() == 0 )
                {
                    log("POCHI", LOGLEVEL_ERROR, "WATSON HTTP POST FAILED!\r\n");
                }
                else
                {
                    log("POCHI", LOGLEVEL_MARK, "WATSON HTTP POST SUCCESS!. reslen=%d\r\n", 
                        response.size());
                }
            }
        }
        Thread::wait(5000);
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
