#include "Display.h"

static U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
static float temp = 0; 
static char msg[32] = "";

void display_begin() {
    u8g2.begin();
}

void display_setTemp(float t) {
   temp = t; 
}

void display_setMessage(const char* str){
    strncpy(msg, str, sizeof(msg) - 1);
}

void display_clearAlert(){    
    msg[0] = '\0';
}

void update_display(){
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    u8g2.drawStr(0, 12, msg);
    if (msg[0] != '\0')
        u8g2.drawStr(0, 28, ("Temp: " + String(temp, 1) + "C").c_str());

    u8g2.sendBuffer();
}