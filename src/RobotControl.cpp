
#include "RobotControl.h"

#include "common.h"
#include "logprint.h"

#include "SoundControl.h"

#define WATSON_POST_URL "http://gateway-a.watsonplatform.net/visual-recognition/api/v3/classify?api_key=35b8434adfad0549dcb8f12a34d79e99ffc29493&version=2016-05-20"
#define RKING_POST_URL "http://104.199.222.173/r-king/watson/uploadapi.php"

#define TEST_WIFI_SSID "4CE67630E22B"
#define TEST_WIFI_PASS "t3340pn5mkmkh"

static char g_ssid[PATH_MAX];
static char g_pass[PATH_MAX];
static char g_post_url[PATH_MAX];

#define watson_classid "{\r\n   \"classifier_ids\":[\r\n      \"CUSTOM_497729065\"\r\n      \r\n   ],\r\n \"threshold\":0.3}\r\n"
bool is_connect_wifi = false;

static DigitalIn s_UserSW(USER_BUTTON0);

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
 * main処理.
 * @param
 * @return 1(1):成功, FALSE(0):失敗
 */
bool RobotControl::powerOn()
{
    log("POCHI", LOGLEVEL_MARK, "POWER ON\r\n");

    //-----------------------------------
    // 設定ファイル読み込み
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

        log("POCHI", LOGLEVEL_INFO, "SETTING FILE READ SUCCESS! %s, %s, %s\r\n",
            g_ssid,
            g_pass,
            g_post_url);
    }
    else
    {
        log("POCHI", LOGLEVEL_WARN, "SETTING FILE NOT FOUND!\r\n");

        strcpy(g_ssid, TEST_WIFI_SSID);
        strcpy(g_pass, TEST_WIFI_PASS);
        strcpy(g_post_url, WATSON_POST_URL);
    }

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

    log("POCHI", LOGLEVEL_INFO, "TAKE PICTURE MODE. PLEASE PUSH USER0 BUTTON.. \r\n");

    while (m_isLife)
    {
        //-----------------------------------
        // カメラ撮影
        //-----------------------------------
        if (m_pCameraControl != NULL)
        {
            if( s_UserSW == 1 ){
                Thread::wait(100);
                continue;
            }

            char filename[PATH_MAX];
            sprintf(filename, "/" MOUNT_NAME "/img%d.jpg", filecnt++);
            if (m_pCameraControl->takeCamera(filename) == 1)
            {
                log("POCHI", LOGLEVEL_INFO, "CAMERA TAKE PICTURE SUCCESS. %s\r\n", filename);

                string response;
                string jsonstr;

                response = postMultiFileToServer((char *)WATSON_POST_URL, filename, watson_classid);
                if (response.size() == 0 )
                {
                    log("POCHI", LOGLEVEL_ERROR, "HTTP POST FAILED! %d %s\r\n", response.size(), response.c_str());
                }
                else
                {
                    log("POCHI", LOGLEVEL_MARK, "HTTP POST SUCCESS!. reslen=%d\r\n", 
                        response.size());
                    jsonstr = response;
                }

                Thread::wait(500);

                response = postMultiFileToServer((char *)RKING_POST_URL, filename, jsonstr.c_str());
                if (response.size() == 0 )
                {
                    log("POCHI", LOGLEVEL_ERROR, "WATSON HTTP POST FAILED!\r\n");
                }
                else
                {
                    log("POCHI", LOGLEVEL_MARK, "WATSON HTTP POST SUCCESS!. reslen=%d\r\n", 
                        response.size());
                }

                Thread::wait(500);
                
                if (jsonstr.find("LYCHEE") != std::string::npos)
                {
                    log("POCHI", LOGLEVEL_INFO, "THIS IS LYCHEE!\r\n");
                    playSound("/storage/lychee.wav");
                }
                else if (jsonstr.find("PEACH") != std::string::npos)
                {
                    log("POCHI", LOGLEVEL_INFO, "THIS IS PEACH!\r\n");
                    playSound("/storage/peach.wav");
                }
                else
                {
                    log("POCHI", LOGLEVEL_INFO, "THIS IS UNKNOWN!\r\n");
                    playSound("/storage/unknown.wav");
                }
            }
            else
            {
                log("POCHI", LOGLEVEL_INFO, "CAMERA TAKE PICTURE FAILED. %s\r\n", filename);
                Thread::wait(1000);
                continue;
            }
        }
        Thread::wait(5000);
    }
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
        char s_contdisp[] = "Content-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\n";
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

        //delete response;
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
    const char* json_str = filename2;

    FILE *fp_jpeg = fopen(filename, "r");
    if ( (fp_jpeg != NULL) )
    {
        fseek(fp_jpeg, 0, SEEK_END);
        int jpeg_len = ftell(fp_jpeg);
        fseek(fp_jpeg, 0, SEEK_SET);
        
        log("POCHI", LOGLEVEL_DEBUG, "IMAGE FILE SIZE = %d\r\n", jpeg_len);

        // create post header
        char s_boundary[] = "------WebKitFormBoundaryZRUHAuSd1pDiYfK5\r\n";
        char s_contdisp[] = "Content-Disposition: form-data; name=\"image_files\"; filename=\"cam_000.jpg\"\r\n";
        char s_conttype[] = "Content-Type: image/jpeg\r\n\r\n";
        char e_boundary[] = "\r\n------WebKitFormBoundaryZRUHAuSd1pDiYfK5--";
        char s_jsondisp[] = "Content-Disposition: form-data; name=\"parameters\"; filename=\"watson.json\"\r\n";
        char s_jsontype[] = "Content-Type: application/json\r\n\r\n";
        
        int sendlen = strlen(s_boundary) + strlen(s_contdisp) + strlen(s_conttype) + jpeg_len + strlen(e_boundary);
        char *reqbuf = new char[sendlen+strlen(s_boundary)+strlen(s_jsondisp)+strlen(s_jsontype)+strlen(json_str)+strlen(s_boundary)];
        char *imgbuf = new char[jpeg_len];
        
        int  reqlen = 0;
                      
        memcpy(reqbuf, s_boundary, strlen(s_boundary));
        reqlen = strlen(s_boundary);

        memcpy(&reqbuf[reqlen], s_contdisp, strlen(s_contdisp));
        reqlen += strlen(s_contdisp);

        memcpy(&reqbuf[reqlen], s_conttype, strlen(s_conttype));
        reqlen += strlen(s_conttype);

        fread(imgbuf, 1, jpeg_len, fp_jpeg);
        fclose(fp_jpeg);
        
        memcpy(&reqbuf[reqlen], imgbuf, jpeg_len);
        reqlen += jpeg_len;

          memcpy(&reqbuf[reqlen], "\r\n", strlen("\r\n"));
          reqlen += strlen("\r\n");

        memcpy(&reqbuf[reqlen], s_boundary, strlen(s_boundary));
        reqlen += strlen(s_boundary);

        memcpy(&reqbuf[reqlen], s_jsondisp, strlen(s_jsondisp));
        reqlen += strlen(s_jsondisp);

        memcpy(&reqbuf[reqlen], s_jsontype, strlen(s_jsontype));
        reqlen += strlen(s_jsontype);
        
        memcpy(&reqbuf[reqlen], json_str, strlen(json_str));
        reqlen += strlen(json_str);

        memcpy(&reqbuf[reqlen], e_boundary, strlen(e_boundary));
        reqlen += strlen(e_boundary);

        // make http request

        log("POCHI", LOGLEVEL_MARK, "SEND POST. url=%s\r\n", url);

        m_HttpRequest = new HttpRequest(m_pWifiInterface, HTTP_POST, url);

        m_HttpRequest->set_header("Content-Type", "multipart/form-data; boundary=----WebKitFormBoundaryZRUHAuSd1pDiYfK5");
        m_HttpRequest->set_header("Connection", "keep-alive");
        m_HttpRequest->set_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.99 Safari/537.36");
        HttpResponse* response;

        // send request
        response = m_HttpRequest->send(reqbuf, reqlen);
        log("POCHI", LOGLEVEL_INFO, "POST FILE SIZE = %d\r\n", reqlen);
        
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
//        delete response;
        delete m_HttpRequest; // also clears out the response
    }
    else
    {
        log("POCHI", LOGLEVEL_ERROR, "SEND FILE NOT FOUND!\r\n");
    }

    return resultstr;
}
