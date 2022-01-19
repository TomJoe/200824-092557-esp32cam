
#include "SPIFFS.h"
#include <TimeLib.h>
#include "esp_camera.h"
#include <WiFi.h>

#define LOGFILENAME "/log"

bool espShallRestart = false;
bool startConfigPortal = false;
unsigned long startTime;
const char version[] = "build "  __DATE__ " " __TIME__;
char *hostname;

void setFullhostname(char * _fullhostname){
     Serial.print("got _fullhostname: ");
     Serial.println(_fullhostname);
    hostname = _fullhostname;
};
char *getFullhostname(){
    return hostname;
};

void setEspShallRestart(bool shallIT){
    espShallRestart = shallIT;
}

void setStartConfigPortal(bool shallIT){
    startConfigPortal = shallIT;
}

void setStartTime(unsigned long epochTime){
    startTime = epochTime;
};
unsigned long getStartTime(){
    return startTime;
};

bool getEspShallRestart(){
    return espShallRestart;
}

bool getStartConfigPortal(){
    return startConfigPortal;
}

char *shallUpdateFromUrl(){
    return NULL;
};

char *epochToTimeString(unsigned long offset){
    time_t t = startTime + offset / 1000;
    static char timestring[30];
    sprintf(timestring, "%4.4i-%2.2i-%2.2i %2.2i:%2.2i:%2.2i.%4.4i", year(t),month(t),day(t),hour(t),minute(t),second(t),(int) offset%1000);
    return timestring;
}

char *currentTime(){
    return epochToTimeString(millis());
}

char *uptime(unsigned long milli)
{
  static char _return[32];
  unsigned long secs=milli/1000, mins=secs/60;
  unsigned int hours=mins/60, days=hours/24;
  milli-=secs*1000;
  secs-=mins*60;
  mins-=hours*60;
  hours-=days*24;
  sprintf(_return,"%dd %2.2d:%2.2d:%2.2d.%3.3d", (byte)days, (byte)hours, (byte)mins, (byte)secs, (int)milli);
  return _return;
}

void logToFile(const char *  logtext){
  char logline[128];
  snprintf(logline, 128, "%s : %s", epochToTimeString(millis()), logtext);
  Serial.print("logging: ");
  Serial.println(logline);

  File file = SPIFFS.open(LOGFILENAME, FILE_APPEND);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
   }

   file.println(logline);
   file.close();
}

void logToFile(String logtext){
    logToFile(logtext.c_str());
}

char *getJsonStatus(){
    static char json_response[2048];

    sensor_t * s = esp_camera_sensor_get();
    char * p = json_response;
    *p++ = '{';

    p+=sprintf(p, "\"hostname\":\"%s\",", hostname);
    p+=sprintf(p, "\"uptime\":\"%s\",", uptime(millis()));
    p+=sprintf(p, "\"time\":\"%s\",", currentTime());
    p+=sprintf(p, "\"build\":\"%s\",", version);
    p+=sprintf(p, "\"IP\":\"%s\",", WiFi.localIP().toString().c_str());
    
    p+=sprintf(p, "\"freeheap\":%u,", ESP.getFreeHeap());
    p+=sprintf(p, "\"ssid\":\"%s\",", WiFi.SSID().c_str());
    p+=sprintf(p, "\"rssi\":%d,", WiFi.RSSI());
    p+=sprintf(p, "\"fs_size\":%u,", SPIFFS.totalBytes());
    p+=sprintf(p, "\"fs_used\":%u,", SPIFFS.usedBytes());
    

    p+=sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p+=sprintf(p, "\"quality\":%u,", s->status.quality);
    p+=sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p+=sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p+=sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p+=sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p+=sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p+=sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p+=sprintf(p, "\"awb\":%u,", s->status.awb);
    p+=sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p+=sprintf(p, "\"aec\":%u,", s->status.aec);
    p+=sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p+=sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p+=sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p+=sprintf(p, "\"agc\":%u,", s->status.agc);
    p+=sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p+=sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p+=sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p+=sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p+=sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p+=sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p+=sprintf(p, "\"vflip\":%u,", s->status.vflip);
    p+=sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p+=sprintf(p, "\"dcw\":%u,", s->status.dcw);
    p+=sprintf(p, "\"colorbar\":%u", s->status.colorbar);
    *p++ = '}';
    *p++ = 0;

    return json_response;
}