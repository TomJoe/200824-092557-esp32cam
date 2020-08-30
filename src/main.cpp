
#ifdef ESP32
#define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())
#else
#define ESP_getChipId()   (ESP.getChipId())
#endif

#include <ArduinoJson.h>
#include <WiFiManager.h>
#include "esp_camera.h"
#include "OTA.h"
#include "FS.h"
#include "SPIFFS.h"
#include <PubSubClient.h>
#include <Ticker.h>


#define MAXLEN 64
#define TOPICNAME esp32camera
#define CONFIGNAME "/config.json"
#define LOGFILENAME "/log"
#define LOGLINE Serial.println("\n ------------- \n");
Ticker ticker;

const char version[] = "build "  __DATE__ " " __TIME__;
char fullhostname[MAXLEN];
char mqttTopic[MAXLEN];
char mqttMsg[MAXLEN];
char logMsg[MAXLEN];
char mqtt_server[MAXLEN] = "192.168.0.20";
char mqtt_port[6] = "1883";
char hostname[MAXLEN] = "skyNETCam";

int notConnectedSinceSeconds;
int noStatusSinceSeconds;
WiFiManager wifiManager;
WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, MAXLEN);
WiFiManagerParameter custom_hostName("hostname", "camera Network name", hostname, MAXLEN);

//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

char *uptime(unsigned long milli)
{
  static char _return[32];
  unsigned long secs=milli/1000, mins=secs/60;
  unsigned int hours=mins/60, days=hours/24;
  milli-=secs*1000;
  secs-=mins*60;
  mins-=hours*60;
  hours-=days*24;
  sprintf(_return,"Uptime %d days %2.2d:%2.2d:%2.2d.%3.3d", (byte)days, (byte)hours, (byte)mins, (byte)secs, (int)milli);
  return _return;
}

WiFiClient espClient;
PubSubClient client(espClient);
void setUpMqtt(){
  client.setServer(mqtt_server, 1883);
      // Attempt to connect
    if (client.connect(hostname)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      snprintf(mqttTopic, MAXLEN, "esp32camera/%s/IP",hostname);
      client.publish(mqttTopic,  WiFi.localIP().toString().c_str());
      
      snprintf(mqttTopic, MAXLEN, "esp32camera/%s/MAC",hostname);
      uint8_t mac[6];
      WiFi.macAddress(mac);
      snprintf(mqttMsg, MAXLEN, "%02x-%02x-%02x-%02x-%02x-%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      client.publish(mqttTopic,  mqttMsg);
      
      snprintf(mqttTopic, MAXLEN, "esp32camera/%s/UPTIME",hostname);
      snprintf(mqttMsg, MAXLEN, "%i", (millis() / 1000));
      client.publish(mqttTopic,  mqttMsg);

      snprintf(mqttTopic, MAXLEN, "esp32camera/%s/STREAMURL",hostname);
      snprintf(mqttMsg, MAXLEN, "http://%s:81/stream", WiFi.localIP().toString().c_str());
      client.publish(mqttTopic,  mqttMsg);

      snprintf(mqttTopic, MAXLEN, "esp32camera/%s/RSSI",hostname);
      snprintf(mqttMsg, MAXLEN, "%i", WiFi.RSSI());
      client.publish(mqttTopic,  mqttMsg);

      snprintf(mqttTopic, MAXLEN, "esp32camera/%s/BSSSI",hostname);
      snprintf(mqttMsg, MAXLEN, "%i", WiFi.BSSIDstr());
      client.publish(mqttTopic,  mqttMsg);


      snprintf(mqttTopic, MAXLEN, "esp32camera/%s/SSID",hostname);
      snprintf(mqttMsg, MAXLEN, "%s", WiFi.SSID());
      client.publish(mqttTopic,  mqttMsg);

      snprintf(mqttTopic, MAXLEN, "esp32camera/%s/VERSION",hostname);
      snprintf(mqttMsg, MAXLEN, "%s", version);
      client.publish(mqttTopic,  mqttMsg);

    } else {
      Serial.print("not connected to: "); Serial.println(mqtt_server);
    }
}

void startCameraServer(); //located in app_httpd.cpp

void blink(int blinks, int millisOn, int millisOff){
  pinMode (4, OUTPUT);
  for(int i=0; i<blinks;i++){
    digitalWrite(4,HIGH);
    delay(millisOn);
    digitalWrite(4,LOW);
    delay(millisOff);
  }
}

void tick()
{
  blink(1,10,0);
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readFile(fs::FS &fs, const char * path){
    LOGLINE
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
    LOGLINE
}

void setupCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_XGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif
}

// Loads the configuration from a file
void loadConfiguration() {
  // Open file for reading
  File file = SPIFFS.open(CONFIGNAME);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  // Copy values from the JsonDocument to the Config
  //mqtt_port = doc["mqtt_port"] | 1883;
  strlcpy(hostname,                  // <- destination
          doc["hostname"] | fullhostname,  // <- source
          sizeof(hostname));         // <- destination's capacity

  strlcpy(mqtt_server,                  // <- destination
          doc["mqtt_server"] | "192.168.0.20",  // <- source
          sizeof(mqtt_server));         // <- destination's capacity

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();

  Serial.println("Config read: \n");
  Serial.print("hostname: "); Serial.println(hostname); 
  Serial.print("mqtt_server: "); Serial.println(mqtt_server); 
  Serial.println("---------------\n");
}

void logToFile(const char *  logtext){
  File file = SPIFFS.open(LOGFILENAME, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
   }
   file.println(logtext);
}

void whatHappened(){
  File file = SPIFFS.open(LOGFILENAME, FILE_READ);
  if (!file) {
    Serial.println(F("Failed to open logfile"));
    return;
   }

  while(file.available()){
    String logLine = file.readStringUntil('\n');
    snprintf(mqttTopic, MAXLEN, "esp32camera/%s/LOG",hostname);
    snprintf(mqttMsg, MAXLEN, "%s", logLine.c_str());
    client.publish(mqttTopic,  mqttMsg);
   }
}

// Saves the configuration to a file
void saveConfiguration() {
  LOGLINE
  Serial.println("saving Config");

  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(hostname, custom_hostName.getValue());

  Serial.print("hostname: "); Serial.println(hostname); 
  Serial.print("mqtt_server: "); Serial.println(mqtt_server); 

  LOGLINE
  
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(CONFIGNAME);

  // Open file for writing
  File file = SPIFFS.open(CONFIGNAME, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
   }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Set the values in the document
  doc["hostname"] = hostname;
  doc["mqtt_server"] = mqtt_server;

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(2.0, tick);
}

void setUpConfigPortal(){
    loadConfiguration();
    wifiManager.setHostname(hostname);
    wifiManager.setConfigPortalTimeout(60);
    wifiManager.setSaveConfigCallback(saveConfiguration);
    wifiManager.setAPCallback(configModeCallback);

    custom_hostName.setValue(hostname,  sizeof(hostname));
    custom_mqtt_server.setValue(mqtt_server, sizeof(mqtt_server));
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_hostName);

    //wifiManager.startConfigPortal(fullhostname);
    wifiManager.autoConnect(hostname);
  };

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  blink(2,10,100);

  setupCamera();

  uint8_t mac[6];
  WiFi.macAddress(mac);

  snprintf(fullhostname, MAXLEN, "%s-%02x-%02x-%02x-%02x-%02x-%02x", "SkyNETCam", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  while (!SPIFFS.begin(true)) {
    Serial.println(F("Failed to initialize SD library.. fomartting"));
    delay(1000);
  }

  setUpConfigPortal();

  ticker.detach();
  digitalWrite(4,LOW);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    notConnectedSinceSeconds = 0;

    setupOTA(fullhostname);

    setUpMqtt();

    whatHappened();

    startCameraServer();

    Serial.print("Camera Ready! Use 'http://");Serial.println(WiFi.localIP());
    Serial.print("or Stream under http://");Serial.print(WiFi.localIP());Serial.print(":81/stream");Serial.println("' to connect");



  } else {
    ESP.restart();
  }

  snprintf(logMsg, MAXLEN, "initialized after %i seconds", (long) millis()/1000);
  logToFile(logMsg);
  blink(5,10,100);
}

void loop() {
  ArduinoOTA.handle();

  delay(1000);
  Serial.print(".");
  //client.publish("camrea", fullhostname);
  if(noStatusSinceSeconds++ > 10){
    snprintf(mqttTopic, MAXLEN, "esp32camera/%s/UPTIME",hostname);
    snprintf(mqttMsg, MAXLEN, "%i", (millis() / 1000));
    client.publish(mqttTopic,  mqttMsg);
    noStatusSinceSeconds = 0;
  }

  if (WiFi.status() == WL_CONNECTED) {
    notConnectedSinceSeconds = 0;
  } else {
    Serial.println("ohoh no WIFI!");
    blink(1,1000,0);
    if (notConnectedSinceSeconds++ > 10) {
      Serial.printf("not connected since %i seconds.Rebooting...", notConnectedSinceSeconds);
      snprintf(logMsg, MAXLEN, "restart: signal lost %i seconds", (long) millis()/1000);
      logToFile(logMsg);
      ESP.restart();
    }
  }

}
