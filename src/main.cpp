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


// reset display, print title, and wait a second
// @param title text to print
inline void testTitle(const char* title){
  d.Reset();
  d.clear();
  d.printf("%s",title);
  d.SetPos(0);
  delay(1000);
}

// testing Character RAM, loop all internal characters and values
void testCharRam(void) {
  testTitle("testCharRAM");
  for(uint8_t ch=0; ch<144; ch++) {
    d.WriteText(0,"CharRAM[%03u]=%c",ch,ch);
    delay(100);
  }
  testTitle("test-UDC-RAM");
  for(uint8_t ch=utf8Ascii; ch<utf8chUDC; ch++) {
    d.WriteText(0,"UDC-RAM[%03u]=%c",ch,ch);
    delay(200);
  }
  delay(1000);
  d.clear();
}

// testing Brightness, loop brightness between 0=100% and 6=13%
void testBrightness(void) {
  uint8_t bn_p[8] = { 100u,80u,53u,40u,27u,20u,13u,0u };
  testTitle("testBrightness  ");
  for(uint8_t ic=0; ic<8; ic++) {
    d.WriteText(0,"Bright[%u] = %3u%%",ic,bn_p[ic]);
    d.SetBrightness(ic);
    delay(1000);
  }
  d.SetBrightness(4);
}

// test Flashing Mode, flash different positions
void testFlashingText(void) {
  testTitle("testFlashingText");
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
  testTitle("BlinkingText=OFF");
  d.BlinkMode(0);
  delay(2500);
  d.WriteText(13,"ON ");
  d.BlinkMode(1);
  delay(5000);
  d.BlinkMode(0);
  d.WriteText(13,"OFF");
  delay(2500);
  d.clear();
}

// starts built-in self-test for left and right display
void doSelftest(void) {
  uint8_t res=0; 
  testTitle("self-test left  ");
  d.clear();
  d.WriteText(8," <--Test");
  res=d.Selftest(0); // self-test left display
  d.WriteText(12,"%4s",((1==res)? "OK":"FAIL"));
  delay(2000);  
  testTitle("self-test right "); 
  d.clear();
  d.WriteText(0,"Test--> ");
  res=d.Selftest(1); // self-test right display
  d.WriteText(0,"%-4s",((1==res)? "OK":"FAIL"));
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
  testTitle("test UTF8-string"); 
  d.clear(); 
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
// @param alpha  character set for wipe animation
// @param size   number of characters
// @param speed  delay between char changes
void wiper(const uint8_t *alpha, uint8_t size, uint32_t speed) {
  for(uint8_t ic=0; ic<maxPOS; ic++) {
    for(uint8_t jc=0; jc<size; jc++) {
      d.WriteChar(ic,alpha[jc]);
      delay(speed);
    }
    d.WriteChar(ic,' ');
  }
}

// draws an arrow
// @param alpha character set for arrow
// @param size  number of characters
// @param pos   position within display
// @param del   delay after drawing
void arrow(const uint8_t *alpha, uint8_t size, uint8_t pos, uint32_t del) {
  d.SetPos(pos); 
  for(uint8_t ic=0;ic<size;ic++) {
    d.WriteChar(alpha[ic]);
  }
  delay(del);
}

// animated propellor
// @param alpha  character set for propellor
// @param size   number of characters
// @param pos    position within display
// @param repeat number of repeats
// @param speed  delay between char changes
void prope(const uint8_t *alpha, uint8_t size, uint8_t pos, uint8_t repeat, uint32_t speed) {
  for(uint8_t ic=0;ic<repeat;ic++) {
    for(uint8_t jc=0;jc<size;jc++) {
      d.WriteChar(pos,alpha[jc]); 
      delay(speed);
    }
  }
}

// test user defined fonts
void testUDC(void){
  testTitle("UDC_font");
  prope(prop0,4,10,5,150);
  prope(prop1,8,10,5,150);
  prope(prop3,4,10,2,1000);
  arrow(arro0,3,10,2000);
  arrow(arro1,3,10,2000);
  arrow(arro2,3,10,2000);
  arrow(arro3,3,10,2000);
  arrow(arro4,3,10,2000);
  arrow(arro5,3,10,2000);
}

// test second user defined font with wipe 
void testUDCwipe(void){
  d.SetUdcFont(UDC_wipe,UDC_nch);  // init wipe user defined characters
  testTitle("UDC_wipe");
  prope(prop2,4,10,8,250);
  d.clear(); 
  d.printf("UDC_wipe--tester"); 
  wiper(wipe0,5,50); 
  delay(1000);
  d.clear(); 
  d.printf("UDC_wipe--tester"); 
  wiper(wipe1,7,50); 
  delay(1000);
  d.SetUdcFont(UDC_font,UDC_nch); // init standard user defined characters
}

 void setup() {
  d.Begin();                          // does all the init stuff (SPI, MCP23s17, HDSP2112)
  d.SetUdcFont(UDC_font,UDC_nch);     // init user defined characters
  d.Reset();                          // reset display
  d.WriteText(0,"Hdsp2112-Display");  // say hello
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
}
