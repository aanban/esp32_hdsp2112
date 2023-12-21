#include "hdsp2112.h"
#include "hdsp2112_udc_font.h"

///< user defined SPI interface, here ESP32 standard SPI interface (VSPI)
constexpr int8_t s_cs   =  5;
constexpr int8_t s_clk  = 18;
constexpr int8_t s_mosi = 23;
constexpr int8_t s_miso = 19;
HDSP2112 d(s_cs,s_clk,s_mosi,s_miso); // create instance with user defined SPI

///< standard SPI interface 
// HDSP2112 d;    // create default instance with SPI instance from SPI.h

// testing Character RAM, loop all internal characters and values
void testCharRam(void) {
  d.Reset();d.printf("testCharRAM"); delay(1000); d.SetPos(0); 
  for(uint8_t ch=0; ch<144; ch++) {
    d.SetPos(0);
    d.printf("CharRAM[%03u]=%c",ch,ch);
    delay(100);
  }
  d.Reset();d.printf("test-UDC-RAM"); delay(1000); d.SetPos(0); 
  for(uint8_t ch=utf8Ascii; ch<utf8chUDC; ch++) {
    d.SetPos(0);
    d.printf("UDC-RAM[%03u]=%c",ch,ch);
    delay(200);
  }
  delay(1000);
  d.clear();
}

// testing Brightness, loop brightness between 0=100% and 6=13%
void testBrightness(void) {
  uint8_t bn_p[8] = { 100u,80u,53u,40u,27u,20u,13u,0u };
  d.Reset();d.printf("testBrightness  "); delay(1000); d.SetPos(0);
  for(uint8_t ic=0; ic<8; ic++) {
    d.SetPos(0);
    d.printf("Bright[%u] = %3u%%",ic,bn_p[ic]);
    d.SetBrightness(ic);
    delay(1000);
  }
  d.SetBrightness(4);
}

// test Flashing Mode, flash different positions
void testFlashingText(void) {
  d.Reset();d.printf("testFlashingText");
  d.FlashMode(1);
  d.SetFlashBits(0b1100000000000011);delay(5000);
  d.SetFlashBits(0b0111000000001110);delay(5000);
  d.SetFlashBits(0b0001110000111000);delay(5000);
  d.SetFlashBits(0b0000011111100000);delay(5000);
  d.SetFlashBits(0b0000000110000000);delay(5000);
  d.FlashMode(0);
  d.clear();
}

// test Blinking Mode, both display will blink synchronous
void testBlinkingText(void) {
  d.Reset();
  d.printf("BlinkingText=OFF");
  d.BlinkMode(0);
  delay(2500);
  d.SetPos(13);
  d.printf("ON ");
  d.BlinkMode(1);
  delay(5000);
  d.BlinkMode(0);
  d.SetPos(13);
  d.printf("OFF");
  delay(2500);
  d.clear();
}

// starts built-in self-test for left and right display
void doSelftest(void) {
  uint8_t res=0; 
  d.Reset();d.printf("self-test left  "); delay(2000); 
  d.clear();d.printf("         <--Test");
  res=d.Selftest(0); // self-test left display
  d.SetPos(12);d.printf("%4s",((1==res)? "OK":"FAIL"));
  delay(2000);  
  d.clear();d.printf("self-test right "); delay(2000); 
  d.clear();d.printf("Test-->         ");
  res=d.Selftest(1); // self-test right display
  d.SetPos(0);d.printf("%-4s",((1==res)? "OK":"FAIL"));
  delay(2000);
  d.Reset();         // after self-test a reset is needed
}

constexpr int32_t n_utf8=23;         // number of utf8 strings
const char* utf8_str[n_utf8] = {     // utf8 strings for testing
  "Ä","ä","Ö","ö","Ü","ü","Å","å",
  "α","β","δ","Δ","Ω","Σ","ß","µ",
  "π","λ","η","θ","τ","Γ","Φ"
};

// tests utf8 strings, internally the function UTF8_to_HDSP() maps 
// the utf8 characters to the internal character-RAM location [0..31]
void testUtf8String(void){
  d.Reset();d.printf("test UTF8-string"); delay(2000); d.clear(); 
  for(int32_t ic=0;ic<n_utf8;ic++) {
    d.printf("%s",utf8_str[ic]);
    delay(300);
    if(15 == (ic&15)) {
      d.clear();
    }
  }
  delay(3000);
}

// propellors
constexpr uint8_t prop0[4]={'|','/','-','\\'};
constexpr uint8_t prop1[8]={128,129,130,131,132,133,134,135};
constexpr uint8_t prop2[4]={128,129,130,131};
constexpr uint8_t prop3[4]={aUp,aRight,aDown,aLeft};
// wipers
constexpr uint8_t wipe1[7]={132,133,134,135,136,137,138};
constexpr uint8_t wipe0[5]={139,140,141,142,143};
// arrows
constexpr uint8_t arro0[3]={tLeft,tBar,tBar};
constexpr uint8_t arro1[3]={tBar,tBar,tRight};
constexpr uint8_t arro2[3]={tLeft,'-','-'};
constexpr uint8_t arro3[3]={'-','-',tRight};
constexpr uint8_t arro4[3]={aLeft,'-','-'};
constexpr uint8_t arro5[3]={'-','-',aRight};

// animated wipe out chars
void wiper(const uint8_t *alpha, uint8_t size, uint32_t speed) {
  for(uint8_t ic=0; ic<maxPOS; ic++) {
    for(uint8_t jc=0; jc<size; jc++) {
      d.SetPos(ic);
      d.WriteChar(alpha[jc]);
      delay(speed);
    }
    d.SetPos(ic); d.WriteChar(' ');
  }
}

// draws a bold arrow
void arrow(const uint8_t *alpha, uint8_t size, uint8_t pos) {
  d.SetPos(pos); 
  for(uint8_t ic=0;ic<size;ic++) {
    d.WriteChar(alpha[ic]);
  }
}

// animated propellor
void propellor(const uint8_t *alpha, uint8_t size, uint8_t pos, uint8_t repeat, uint32_t speed) {
  for(uint8_t ic=0;ic<repeat;ic++) {
    for(uint8_t jc=0;jc<size;jc++) {
      d.SetPos(pos); 
      d.WriteChar(alpha[jc]); 
      delay(speed);
    }
  }
}

// test user defined fonts
void testUDC(void){
  d.Reset(); 
  d.printf("UDC_font");
  propellor(prop0,4,10,5,150);
  propellor(prop1,8,10,5,150);
  propellor(prop3,4,10,2,1000);
  arrow(arro0,3,10); delay(2000);
  arrow(arro1,3,10); delay(2000);
  arrow(arro2,3,10); delay(2000);
  arrow(arro3,3,10); delay(2000);
  arrow(arro4,3,10); delay(2000);
  arrow(arro5,3,10); delay(2000);
}

// test second user defined font with wipe 
void testUDCwipe(void){
  d.SetUdcFont(UDC_wipe,UDC_nch);  // init wipe user defined characters
  d.printf("UDC_wipe");
  propellor(prop2,4,10,8,250);
  d.clear(); d.printf("UDC_wipe--tester"); wiper(wipe0,5,50); delay(1000);
  d.clear(); d.printf("UDC_wipe--tester"); wiper(wipe1,7,50); delay(1000);
  d.SetUdcFont(UDC_font,UDC_nch); // init standard user defined characters
}


 void setup() {
  d.Begin();                      // does all the init stuff (SPI, MCP23s17, HDSP2112)
  d.SetUdcFont(UDC_font,UDC_nch); // init user defined characters
  d.Reset();                      // reset display
  d.printf("Hdsp2112-Display");   // say hello
  delay(2000);
}

void loop() {
  testUDC();
  testUDCwipe();
  testUtf8String();
  testFlashingText();
  testCharRam();
  testBlinkingText();
  testBrightness();
  doSelftest();
  d.SetUdChar(UDC_font,0); // workarround for char 128
}
