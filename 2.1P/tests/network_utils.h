#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

bool connectToWifi(void);
void connectToThingSpeak(void);
void sendDataThingSpeak(float t, int l);

#endif