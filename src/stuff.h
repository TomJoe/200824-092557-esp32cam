#ifndef STUFF_H
#define STUFF_H

bool getEspShallRestart();
void setEspShallRestart(bool);
void setStartConfigPortal(bool);
bool getStartConfigPortal();
void setStartTime(unsigned long);
unsigned long getStartTime();
char *uptime(unsigned long);
void logToFile(const char *);
void logToFile(String);
char *currentTime();
char *getJsonStatus();
void setFullhostname(char *);
char *getFullhostname();
char *shallUpdateFromUrl();

#endif