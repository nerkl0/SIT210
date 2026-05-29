#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>

void display_begin();
void display_clearAlert();
void display_setTemp(float t);
void display_setMessage(const char* msg);
void update_display();

#endif