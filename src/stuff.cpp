
#include "SPIFFS.h"
#include <TimeLib.h>

#define LOGFILENAME "/log"

bool espShallRestart = false;
unsigned long startTime;

void setEspShallRestart(bool shallIT){
    espShallRestart = shallIT;
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

String epochToTimeString(unsigned long offset){
    time_t t = startTime + offset / 1000;
    char timestring[30];
    sprintf(timestring, "%4i-%2i-%2i-%2i-%2i-%2i-%4i", (int) year(t),month(t),day(t),hour(t),minute(t),second(t),offset%1000);
    return timestring;
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