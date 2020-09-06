#ifndef FILELOGGER_H
#define FILELOGGER_H
#include <SPIFFS.h>

#define LOGFILENAME "/log"

static char *uptime(unsigned long milli)
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

static void logToFile(const char *  logtext){
  char logline[128];
  snprintf(logline, 128, "%s : %s", uptime(millis()), logtext);
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

static void logToFile(String logtext){
    logToFile(logtext.c_str());
}

#endif