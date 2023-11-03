#include "hdsp2112.h"

HDSP2112 d; // instance of hdps2112 display driver

// testing Character RAM, loop all internal characters and values
void testChars() {
  d.Reset();
  for(uint8_t ch=0; ch<64; ch++) {
    d.SetPos(0);
    d.printf("%03d %c   %03d %c",ch,ch,ch+64,ch+64);
    delay(200);
  }
}

// testing Brightness, adjust brightness between 0=100% 7=0%
void testBrightness() {
  d.Reset();
  for(uint8_t icnt=0; icnt<7;icnt++) {
    d.SetPos(0);
    d.printf("Brightness = %u",icnt);
    d.SetBrightness(icnt);
    delay(2000);
  }
}

// test Flashing Mode
void testFlashing() {
  d.Reset(); 
  d.printf("testFlashing1234");
  delay(1000);
  d.SetFlashBits(0b1111000000000000);
  d.FlashMode(1);
  delay(5000);
  d.SetFlashBits(0b0000111100000000);
  delay(5000);
  d.SetFlashBits(0b0000000011110000);
  delay(5000);
  d.SetFlashBits(0b0000000000001111);
  delay(5000);
  d.FlashMode(0);
}

// test Blinking Mode 
void testBlinking() {
  d.Reset(); 
  d.printf("Blinking        ");
  delay(1000);
  d.BlinkMode(1);
  d.SetPos(11);d.printf(" ON ");
  delay(5000);
  d.BlinkMode(0);
  d.SetPos(11);d.printf(" OFF");
  delay(5000);
}

void setup() {
  Serial.begin(115200);
  printf("starting display..");
  d.Begin();
  printf("ok\n");
}

void loop() {
  testChars();
  testBrightness();
  testFlashing();
  testBlinking();
}
