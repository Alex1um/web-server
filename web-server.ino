#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>
// #include <CircularBuffer.h>

#define ESP_MODEL_WROOM
#if defined(CAMERA_MODEL_AI_THINKER)
  #define MOTOR_1_PIN_1    14
  #define MOTOR_1_PIN_2    15
  #define MOTOR_2_PIN_1    13
  #define MOTOR_2_PIN_2    12
  #define LED_PIN 33
  #define LED_HIGH LOW
  #define LED_LOW HIGH
#elif defined(ESP_MODEL_WROOM)
  #define MOTOR_1_PIN_1    32
  #define MOTOR_1_PIN_2    33
  #define MOTOR_2_PIN_1    25
  #define MOTOR_2_PIN_2    26
  #define LED_PIN 2
  #define LED_HIGH HIGH
  #define LED_LOW LOW
  #define SDA_PIN 21
  #define SCL_PIN 22
#endif

WebServer server(80);

const char *ssid = "Hackspace";
const char *pass = "youathackspace";
// const char *ssid = "just another spot";
// const char *pass = "11111111";

MPU9250_asukiaaa Sensor;

void go_forward() {
  digitalWrite(MOTOR_1_PIN_1, 1);
  digitalWrite(MOTOR_1_PIN_2, 0);
  digitalWrite(MOTOR_2_PIN_1, 1);
  digitalWrite(MOTOR_2_PIN_2, 0);
}

void go_left() {
  digitalWrite(MOTOR_1_PIN_1, 0);
  digitalWrite(MOTOR_1_PIN_2, 1);
  digitalWrite(MOTOR_2_PIN_1, 1);
  digitalWrite(MOTOR_2_PIN_2, 0);
}

void go_right() {
  digitalWrite(MOTOR_1_PIN_1, 1);
  digitalWrite(MOTOR_1_PIN_2, 0);
  digitalWrite(MOTOR_2_PIN_1, 0);
  digitalWrite(MOTOR_2_PIN_2, 1);
}

void go_backward() {
  digitalWrite(MOTOR_1_PIN_1, 0);
  digitalWrite(MOTOR_1_PIN_2, 1);
  digitalWrite(MOTOR_2_PIN_1, 0);
  digitalWrite(MOTOR_2_PIN_2, 1);
}

void stop() {
  digitalWrite(MOTOR_1_PIN_1, 0);
  digitalWrite(MOTOR_1_PIN_2, 0);
  digitalWrite(MOTOR_2_PIN_1, 0);
  digitalWrite(MOTOR_2_PIN_2, 0);
}

void go(String dir) {
  if (dir.equals("forward")) {
    go_forward();
  } else if (dir.equals("backward")) {
    go_backward();
  } else if (dir.equals("left")) {
    go_left();
  } else if (dir.equals("right")) {
    go_right();
  } else {
    stop();
  }
}

void led_on() {
  digitalWrite(LED_PIN, LED_HIGH);
}

void led_off() {
  digitalWrite(LED_PIN, LED_LOW);
}

// struct AccelData {
//   float x;
//   float y;
//   float z;
//   float sqrt;
// };

// CircularBuffer<AccelData, 10> accelBuf;

// uint32_t listen(uint32_t time) {
//   unsigned long start = millis();
//   accelBuf.clear();
//   while (millis() - start < time) {
//     Sensor.accelUpdate();
//     accelBuf.push(
//       AccelData{ 
//         .x=Sensor.accelX(),
//         .y=Sensor.accelY(),
//         .z=Sensor.accelZ(),
//         .sqrt=Sensor.accelSqrt(),
//       }
//     );

//   }
//   return 0;
// }

float filtVal = 0.;
uint32_t listen(uint32_t time) {
  unsigned long start = millis();
  while (millis() - start < time) {
    Sensor.accelUpdate();
    float newVal = (pow(Sensor.accelY(), 2) - filtVal) * .1;
    if (newVal > .3) return 400;
    filtVal += newVal;
  }
  return 200;
}

void action() {
  String direction;
  String uri = server.uri();
  String dest = uri.substring(uri.lastIndexOf("/") + 1);
  Serial.println(server.uri());
  Serial.println(server.args());
  bool is_delayed = server.argName(0).equals("delay") && !dest.equals("stop");
  go(dest);
  if (is_delayed) {
    int code = 200;
    if (dest == "left" || dest == "right") {
      delay(server.arg(0).toInt());
    } else {
      code = listen(server.arg(0).toInt());
    }
    stop();
    server.send(code);
  }
}

void get_gyro() {
  Sensor.gyroUpdate();
  char tmp[200];
  sprintf(tmp, "\
  {\
    \"x\": %f,\
    \"y\": %f,\
    \"z\": %f\
  }\
  ",
  Sensor.gyroX(), 
  Sensor.gyroY(),
  Sensor.gyroZ());
  server.send(200, "application/json", tmp);
}

void get_accel() {
  Sensor.accelUpdate();
  char tmp[200];
  sprintf(tmp, "\
  {\
    \"x\": %f,\
    \"y\": %f,\
    \"z\": %f,\
    \"sqrt\": %f\
  }\
  ",
  Sensor.accelX(), 
  Sensor.accelY(), 
  Sensor.accelZ(), 
  Sensor.accelSqrt());
  server.send(200, "application/json", tmp);
}

void setup()
{
    Serial.begin(115200);

    pinMode(MOTOR_1_PIN_1, OUTPUT);
    pinMode(MOTOR_1_PIN_2, OUTPUT);
    pinMode(MOTOR_2_PIN_1, OUTPUT);
    pinMode(MOTOR_2_PIN_2, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    Wire.begin(SDA_PIN, SCL_PIN);
    Sensor.setWire(&Wire);

    Sensor.beginGyro();
    Sensor.beginAccel();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/go/forward", action);
    server.on("/go/backward", action);
    server.on("/go/left", action);
    server.on("/go/right", action);
    server.on("/go/stop", action);
    server.on("/led/on", led_on);
    server.on("/led/off", led_off);
    server.on("/info/accel", get_accel);
    server.on("/info/gyro", get_gyro);
    server.enableCORS();
    server.begin();
    Serial.println("HTTP server started");

}

void loop()
{
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks
}

void yield()
{
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks
}
