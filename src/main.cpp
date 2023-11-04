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
void testChars(void) {
  d.Reset();
  d.SetBrightness(5);
  for(uint8_t ch=0; ch<128; ch++) {
    d.SetPos(0);
    d.printf("val=%03d char=%c",ch,ch);
    delay(100);
  }
}

// testing Brightness, loop brightness between 0=100% and 6=13%
void testBrightness(void) {
  uint8_t bn_p[8] = { 100u,80u,53u,40u,27u,20u,13u,0u };
  d.clear();
  for(uint8_t ic=0; ic<8; ic++) {
    d.SetPos(0);
    d.printf("Brightness=%3u%%",bn_p[ic]);
    d.SetBrightness(ic);
    delay(2000);
  }
}

// test Flashing Mode, loop 2-character wise thru left and right display
void testFlashing(void) {
  uint16_t mask=3; // mask for character-positions 0..15
  d.clear();
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
void testBlinking(void) {
  d.clear();
  d.SetPos(0);
  d.printf("Text is blinking");
  d.BlinkMode(1);
  delay(5000);
  d.BlinkMode(0);
  d.SetPos(0);
  d.printf("NonBlinking Text");
  delay(5000);
}

// starts selftest for left and right display
void doSelftest(void) {
  d.clear();
  d.SetPos(8);
  d.printf(" <--Test");
  d.Selftest(0); // do self-test left display
  d.clear();
  d.printf("Test--> ");
  d.Selftest(1); // do self-test right display
  d.clear();
}

void setup() {
  d.Begin();
  d.SetBrightness(4);
  d.printf("Hdsp2112-Display");
  delay(2000);
  doSelftest();
}

void loop() {
  testChars();
  testFlashing();
  testBlinking();
  testBrightness();
}
