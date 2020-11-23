#ifndef STUFF_H
#define STUFF_H

bool getEspShallRestart();
void setEspShallRestart(bool);
void setStartTime(unsigned long);
unsigned long getStartTime();
char *uptime(unsigned long);
void logToFile(const char *);
void logToFile(String);

#endif