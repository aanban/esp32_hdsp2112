#include "hdsp2112.h"

///< user defined SPI interface, here ESP32 standard SPI interface (VSPI)
constexpr int8_t s_cs   =  5;
constexpr int8_t s_clk  = 18;
constexpr int8_t s_mosi = 23;
constexpr int8_t s_miso = 19;
HDSP2112 d(s_cs,s_clk,s_mosi,s_miso); // create instance with user defined SPI

///< standard SPI interface 
// HDSP2112 d;    // create default instance

// testing Character RAM, loop all internal characters and values
void testChars() {
  d.Reset();
  d.SetBrightness(5);
  for(uint8_t ch=0; ch<128; ch++) {
    d.SetPos(0);
    d.printf("val=%03d char=%c",ch,ch);
    delay(100);
  }
}

// testing Brightness, loop brightness between 0=100% and 6=13%
void testBrightness() {
  uint8_t bn_p[8] = { 100u,80u,53u,40u,27u,20u,13u,0u };
  d.ClearDisplay();
  for(uint8_t ic=0; ic<8; ic++) {
    d.SetPos(0);
    d.printf("Brightness=%3u%%",bn_p[ic]);
    d.SetBrightness(ic);
    delay(2000);
  }
}

// test Flashing Mode, loop 2-character wise thru left and right display
void testFlashing() {
  uint16_t mask=3; // mask for character-positions 0..15
  d.ClearDisplay();
  d.printf("FlashingTest1234");
  d.SetFlashBits(mask);
  d.FlashMode(1);
  delay(1000);
  for(uint16_t cnt=0;cnt<16;cnt+=2) {
    d.SetFlashBits(mask << cnt);  
    delay(5000);
  }
  d.FlashMode(0);
}

// test Blinking Mode, both display will blink synchronous
void testBlinking() {
  d.ClearDisplay();
  d.printf("Blinking         ");
  delay(1000);
  d.BlinkMode(1);
  d.SetPos(11);d.printf("  ON");
  delay(5000);
  d.BlinkMode(0);
  d.SetPos(11);d.printf(" OFF");
  delay(5000);
}

void setup() {
  d.Begin();
  d.printf("HDSP2112-Display");
  delay(5000);
}

void loop() {
  testChars();
  testFlashing();
  testBlinking();
  testBrightness();
}
