//
// line 412
// int getDistance = map(analogRead(A0), 0, 1023, 1, 5);
// 若未降低 ADC 讀取頻率，會造成網頁無法開啟
//

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_MLX90614.h>

/***************↓↓↓↓↓ 更改設定值 ↓↓↓↓↓***************/
// WIFI
char ssid[] = "SoftSkillsUnion";
char wifipwd[] = "24209346";
// char ssid[] = "ChungWa_4G";
// char wifipwd[] = "89956399";

// 溫測誤差值
float errorCorrection = 3.5;

// 設定 Static IP address
IPAddress local_IP(192, 168, 107, 188);
IPAddress gateway(192, 168, 107, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);
/***************↑↑↑↑↑ 更改設定值 ↑↑↑↑↑***************/

#define pinLED D4    // 控制 LED
#define pinIR D5     // 控制 TCRT5000 紅外線
#define pinBuzzer D8 // 控制 Buzzer

bool getTempTrigger = 0; // 是否取得偵測溫度
bool getObject = 0;      // 是否偵測到物體
float objTemperature = 0.0;

String json;

int ledBrightness = 10;    // LED 亮度
int detectionDistance = 3; // 判斷偵測到物體的距離
int getDistance;
unsigned long getADCDuration = 5;    // 偵測 ADC 間隔
unsigned long getADCTime = 0;        // 開始偵測 ADC 時間
unsigned long getTempDuration = 500; // 持續偵測到物體時間
unsigned long getObjTime = 0;        // 開始偵測到物體時間

Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // 宣告紅外線測溫物件

ESP8266WebServer server;                           // 宣告 WEB Server 物件
WebSocketsServer webSocket = WebSocketsServer(81); // WEB Server 加入 webSocket 於 port 81

// PROGMEM 是將大量資料從 SRAM 搬到 Flash，當要使用時再從 Flash 搬回來。
char webpage[] PROGMEM = R"=====(
<html>

<head>
    <!-- 正常顯示繁體中文 -->
    <meta charset='utf-8'>
    <!-- 引用外部函式庫 -->
    <!-- <script src='https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.3.3/jscolor.min.js'></script> -->
    <style type="text/css">
        /* https://codepen.io/sdthornton/pen/wBZdXq */
        body {
            background: #5a5a5a;
            text-align: center;
            align-items: center;
            flex-direction: column;
            font-family: sans-serif;
        }

        .card {
            background: rgb(56, 56, 56);
            border-radius: 2px;
            display: inline-block;
            width: 40%;
            min-width: 350px;
            height: 40%;
            min-height: 200px;
            margin: 5px;
            position: relative;
            color: floralwhite;
        }

        .bluecard {
            background: rgba(0, 0, 255, 0.5);
            border-radius: 2px;
            display: inline-block;
            width: 40%;
            min-width: 350px;
            height: 40%;
            min-height: 200px;
            margin: 5px;
            position: relative;
            color: floralwhite;
        }

        .greencard {
            background: rgba(0, 255, 0, 0.5);
            border-radius: 2px;
            display: inline-block;
            width: 40%;
            min-width: 350px;
            height: 40%;
            min-height: 200px;
            margin: 5px;
            position: relative;
            color: floralwhite;
        }

        .redcard {
            background: rgba(255, 0, 0, 0.5);
            border-radius: 2px;
            display: inline-block;
            width: 40%;
            min-width: 350px;
            height: 40%;
            min-height: 200px;
            margin: 5px;
            position: relative;
            color: floralwhite;
        }

        .card-1 {
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.12), 0 1px 2px rgba(0, 0, 0, 0.24);
            transition: all 0.3s cubic-bezier(.25, .8, .25, 1);
        }

        /* .card-1:hover {
            box-shadow: 0 14px 28px rgba(0, 0, 0, 0.25), 0 10px 10px rgba(0, 0, 0, 0.22);
        } */

        /* .card-title {
            position: relative;
        } */

        .title {
            background: #535353;
            border-radius: 5px;
            display: inline-block;
            width: 100%;
            margin: 10px;
            /* overflow: auto; */
            position: relative;
            color: floralwhite;

        }

        .title1 {
            font-size: 35px;
            position: relative;
            float: left;
        }

        .title2 {
            font-size: 25px;
            position: relative;
            float: right;
            top: 15px;
            right: 5px;
        }

        .subtitle {
            display: inline-block;
            width: 100%;
            position: relative;
            color: rgb(190, 190, 190);
            font-size: 18px;
        }

        .subtitle1 {
            position: relative;
            float: left;
        }

        .subtitle2 {
            position: relative;
            float: right;
            right: 5px;
        }

        h1 {
            margin-top: 0.25em;
            margin-bottom: 0;
            position: relative;
        }

        .msg {
            cursor: pointer;
            font-size: 30px;
            display: block;
            position: relative;
            top: 80px;
        }
    </style>

    <title>物聯網遠端控制溫度監測器</title>
</head>

<body>
    <div class="title">
        <span class="title1">物聯網遠端控制溫度監測器</span>
        <a href='https://www.facebook.com/SoftSkillsUnion/' target='_blank' style='color:rgb(190, 190, 190);'><span
                class="title2">軟實力精進聯盟</span></a>
    </div>
    </p>
    <div id="divDistance" class="card card-1">
        <div>
            <h1>監測距離提示</h1>
        </div>
        <div>
            <span class="msg">請將額頭接近感測器<br>直至面板變為藍色</span>
        </div>
    </div>
    <div id="divTemperature" class="card card-1">
        <div>
            <h1>檢測數據顯示</h1>
        </div>
        <div>
            <span class="msg" id="showTemp"><span style="visibility:hidden">調整</span>0.00 ℃<span
                    style="visibility:hidden">高度</span></span>
            <span class="msg" id="showMsg">等待檢測</span>
        </div>
    </div>
    <hr />
    <div class="subtitle">
        <span id="clock" class="subtitle1">Time</span>
        <span class="subtitle2">自學力激發創造力、軟實力提升競爭力</span>
    </div>

    <!-- 客戶端網頁啟用 webSocket -->
    <script>
        var webSocket;
        window.onload = init();
        function addData(objData) {
            var divDistance = document.getElementById("divDistance");
            var divTemperature = document.getElementById("divTemperature");
            var showMsg = document.getElementById("showMsg");
            var temp = objData.objTemperature;
            var limitTemp = 37.4;

            document.getElementById("showTemp").innerText = String(temp) + " ℃";

            if (objData.getObject === 1) {
                divDistance.setAttribute("class", "bluecard card-1");
                if (temp >= limitTemp) {
                    divTemperature.setAttribute("class", "redcard card-1");
                    showMsg.innerText = "體溫過高，請自主隔離";
                } else if (temp != 0.00){
                    divTemperature.setAttribute("class", "greencard card-1");   
                    showMsg.innerText = "體溫正常";             
                }
            } else {
                divDistance.setAttribute("class", "card card-1");
                divTemperature.setAttribute("class", "card card-1");
                showMsg.innerText = "等待檢測";
            }            
        }
        function sendCmnd(cmnd, value) {
            webSocket.send('{"cmnd":"' + cmnd + '","value":"' + value + '"}');
        }
        function init() {
            webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');
            webSocket.onmessage = function (event) {
                var data = JSON.parse(event.data);
                var today = new Date();
                var t = today.getHours() + ":" + today.getMinutes() + ":" + today.getSeconds();
                addData(data);
            }
            showTime();
        }
        function showTime() {
            var today = new Date();
            var hh = today.getHours();
            var mm = today.getMinutes();
            var ss = today.getSeconds();
            mm = checkTime(mm);
            ss = checkTime(ss);
            document.getElementById('clock').innerHTML = hh + ":" + mm + ":" + ss;
            var timeoutId = setTimeout(showTime, 500);
        }
        function checkTime(i) {
            if (i < 10) {
                i = "0" + i;
            }
            return i;
        }
    </script>
</body>


</html>
)=====";

String serializeJson()
{
    String x = "{\"getObject\":";
    x += getObject;
    x += ", \"objTemperature\":\"";
    x += objTemperature;
    x += "\"}";

    Serial.println(x);
    return x;
}

String parseJson(String json, String field)
{
    // 分配空間大小至 https://arduinojson.org/v6/assistant/ 計算
    StaticJsonDocument<100> doc;
    deserializeJson(doc, json);

    return doc[field];
}

void sendJson()
{
    json = serializeJson();
    webSocket.broadcastTXT(json.c_str(), json.length());
}

char *inttochar(int i)
{
    static char c[5];
    sprintf(c, "%d", i);
    // itoa(i, c, 10);
    return c;
}

char *floattochar(float i)
{
    static char c[5];
    sprintf(c, "%3.1f", i);
    return c;
}

char *strtochar(String x)
{
    static char c[50];
    x.toCharArray(c, 50);
    return c;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    Serial.print("WStype = ");
    Serial.println(type);
    Serial.printf("Data Length: %d, Payload: ", length);
    Serial.write(payload, length);
    Serial.println("");

    String pl = "";
    // http://www.martyncurrey.com/esp8266-and-the-arduino-ide-part-9-websockets/
    if (type == WStype_CONNECTED) // 連線成功，回傳初始值
    {
    }
    else if (type == WStype_TEXT) // 接收用戶端訊息做出回應
    {
        for (int i = 0; i < length; i++)
        {
            pl += (char)payload[i];
        }

        String cmnd = parseJson(pl, "cmnd");
        String strValue = parseJson(pl, "value");
        int value = strValue.toInt();

        Serial.print("Cmnd: ");
        Serial.print(cmnd);
        Serial.print(", strValue: ");
        Serial.print(strValue);
        Serial.print(", value: ");
        Serial.println(value);

        // if (cmnd == "switchPump")
        // {
        //     pumpcmnd = value;
        //     if (value)
        //     {
        //         pumpOn();
        //     }
        //     else
        //     {
        //         pumpOff();
        //     }
        // }
        // else if (cmnd == "switchCycle")
        // {
        //     pumpmode = value;
        // }
    }
}

void setup()
{
    pinMode(pinLED, OUTPUT);
    pinMode(pinIR, OUTPUT);
    pinMode(pinBuzzer, OUTPUT);

    // digitalWrite(pinLED, HIGH);
    analogWrite(pinLED, ledBrightness);

    Serial.begin(115200);
    Serial.println();
    Serial.print("connecting to ");
    Serial.println(ssid);

    // 指定靜態IP請將註解移除
    // if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
    // {
    //     Serial.println("STA Failed to configure");
    // }

    WiFi.begin(ssid, wifipwd);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    mlx.begin();

    server.on("/", []() {
        server.send_P(200, "text/html", webpage);
    });
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    digitalWrite(pinLED, LOW);
}

void loop()
{
    webSocket.loop();
    server.handleClient();

    digitalWrite(pinIR, HIGH); // Turn on IR LED

    // 若未降低 ADC 讀取頻率，會造成網頁無法開啟
    if (millis() - getADCTime >= getADCDuration)
    {
        getADCTime = millis();
        getDistance = map(analogRead(A0), 0, 1023, 1, 5);
    }

    // Serial.print("getDistance =");
    // Serial.println(getDistance);

    if (getDistance == detectionDistance) // 距離訊號值為 3，開始測溫
    {
        analogWrite(pinLED, ledBrightness);
        if (!getObject)
        {
            getObject = 1;
            getObjTime = millis();
            sendJson();
            Serial.println("get object");
        }

        if (millis() - getObjTime < getTempDuration)
        {
            // 必須維持距離指定時間後才開始取得溫度
            return;
        }

        if (!getTempTrigger)
        {
            getTempTrigger = 1;

            digitalWrite(pinIR, LOW); // 關閉紅外線測距避免干擾

            objTemperature = mlx.readObjectTempC() + errorCorrection;
            Serial.print(objTemperature);
            Serial.println(" ℃");
            sendJson();

            tone(pinBuzzer, 1000, 100);

            Serial.println("get temperature");
        }
    }
    // 離開偵測距離差值2以上
    else if ((abs(getDistance - detectionDistance) >= 2))
    {
        digitalWrite(pinLED, LOW);
        getObject = 0;
        if (getTempTrigger)
        {
            getTempTrigger = 0;
            objTemperature = 0.0;
            sendJson();
            Serial.println("No Object.");
        }
    }
}
