#include <Wire.h>
#include <MAX30100_PulseOximeter.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Servo.h>

/***************↓↓↓↓↓ 更改設定值 ↓↓↓↓↓***************/
// WIFI
char ssid[] = "SoftSkillsUnion_M";
char wifipwd[] = "24209346";
/***************↑↑↑↑↑ 更改設定值 ↑↑↑↑↑***************/

int servoPin = 15;
float bpm, spO2;

bool isBeat = 0;           // 偵測到心跳
bool switchVisualizer = 1; // 心跳模擬器開關

unsigned long startBeatTime = 0; // 心跳開始時間
int uploadInterval = 5000;       // 上傳數據間隔時間

int posSystole = 55;  // 模擬心室收縮狀態角度
int posDiastole = 90; // 模擬心室舒張狀態角度

PulseOximeter pox;
Servo myservo;

ESP8266WebServer server; // 宣告 WEB Server 物件

WebSocketsServer webSocket = WebSocketsServer(81); // WEB Server 加入 webSocket 於 port 81

// PROGMEM 是將大量資料從 SRAM 搬到 Flash，當要使用時再從 Flash 搬回來。
char webpage[] PROGMEM = R"=====(
<html>

<head>
    <!-- 正常顯示繁體中文 -->
    <meta charset='utf-8'>
    <!-- 引用 Chart.js 數據圖表函式庫 -->
    <script src='https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.5.0/Chart.min.js'></script>
</head>

<body onload="javascript:init()">
    <div>
        心跳模擬器：<label for="switchSlider" id="switchLabel">開</label>
        <input type="range" min="0" max="1" value="1" id="switchSlider" oninput="sendSwitchState()"
            style="width: 30px;" />
    </div>
    <hr />
    <div>
        <canvas id="line-chart" width="800" height="450"></canvas>
    </div>
    <!-- 客戶端網頁啟用 webSocket -->
    <script>
        var webSocket, dataPlot;
        var maxDataPoints = 20;
        function removeData() {
            dataPlot.data.labels.shift();
            dataPlot.data.datasets[0].data.shift();
        }
        function addData(label, bpm, spo2) {
            if (dataPlot.data.labels.length > maxDataPoints) removeData();
            dataPlot.data.labels.push(label);
            dataPlot.data.datasets[0].data.push(bpm);
            dataPlot.data.datasets[1].data.push(spo2);
            dataPlot.update();
        }
        function init() {
            webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');
            dataPlot = new Chart(document.getElementById("line-chart"), {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [{
                        data: [],
                        label: "心率 bpm (次/分鐘)",
                        borderColor: "#3e95cd",
                        fill: false
                    }, {
                        data: [],
                        label: "脈搏血氧濃度 SpO2 (%)",
                        borderColor: "#cd5c5c",
                        fill: false
                    }]
                }
            });
            webSocket.onmessage = function (event) {
                var data = JSON.parse(event.data);
                var today = new Date();
                var t = today.getHours() + ":" + today.getMinutes() + ":" + today.getSeconds();
                addData(t, data.bpm, data.spo2);
            }
        }
        function sendSwitchState() {
            var dataRate = document.getElementById("switchSlider").value;
            var vl = document.getElementById("switchLabel");
            webSocket.send(String(dataRate));
            if (dataRate == 1) {
                vl.innerHTML = "開";
            } else {
                vl.innerHTML = "關";
            }
        }
    </script>
</body>

</html>
)=====";

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);

  // WIFI連線設定
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

  Serial.print("Initializing pulse oximeter..");
  if (!pox.begin())
  {
    Serial.println("FAILED");
    for (;;)
      ;
  }
  else
  {
    Serial.println("SUCCESS");
  }

  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);

  myservo.attach(servoPin);
  myservo.write(posSystole);
  Serial.println("servo done");

  server.on("/", []() {
    server.send_P(200, "text/html", webpage);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  // Make sure to call update as fast as possible
  pox.update();
  webSocket.loop();
  server.handleClient();

  bpm = pox.getHeartRate();
  spO2 = pox.getSpO2();

  if (bpm > 20 && spO2 > 40)
  {
    if (isBeat == 0)
    {
      startBeatTime = millis();
      isBeat = 1;
    }
    if (millis() - startBeatTime > uploadInterval)
    {
      String json = "{\"bpm\":";
      json += bpm;
      json += ", \"spo2\":";
      json += spO2;
      json += "}";
      webSocket.broadcastTXT(json.c_str(), json.length());
      startBeatTime = millis();
      Serial.print("Send Data!");
      Serial.println(json);
      //      Serial.print(bpm);
      //      Serial.print("; ");
      //      Serial.println(spO2);
    }
  }
  else
  {
    isBeat = 0;
  }
}

void onBeatDetected()
{
  Serial.print("Beat! ");
  Serial.print(bpm);
  Serial.print("; ");
  Serial.println(spO2);

  if (switchVisualizer == 1 && isBeat == 1)
  {
    if (myservo.read() == posDiastole)
    {
      myservo.write(posSystole);
    }
    else
    {
      myservo.write(posDiastole);
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  // Do something with the data from the client
  if (type == WStype_TEXT)
  {
    //    float dataRate = (float) atof((const char *) &payload[0]);
    int dataRate = (int)atoi((const char *)&payload[0]);
    switchVisualizer = dataRate;
  }
}
