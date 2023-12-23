#include <hdsp2112.h>
#include <hdsp2112_udc_font.h>

// ------------------------------------------------------------------------
// public members of class
// ------------------------------------------------------------------------ 

HDSP2112::HDSP2112(int8_t spi_cs, int8_t spi_clk, int8_t spi_mosi, int8_t spi_miso) {
  m_spi_cs   = spi_cs;        // SPI chipselect
  m_spi_clk  = spi_clk;       // SPI clock
  m_spi_mosi = spi_mosi;      // SPI master-out-slave-in
  m_spi_miso = spi_miso;      // SPI master-in-slave-out
  m_pos = 0;                  // set cursor to leftmost position
  m_cwr = 0b00000000;         // init hdsp2112 code-word-register
                              //    d7=clear=0 normal operation 
                              //    d6=selftest=0 OFF
                              //    d5=x 
                              //    d4=blink=0 OFF
                              //    d3=flash=0 OFF
                              //    bright=000 = 100%
  m_ctrl = 0b11111111;        // mpc1 port-B = res,fl,wr,rd,cs0,cs1
}

void HDSP2112::Reset(void) {
  m_ctrl &= ~gpbRES & ~gpbCS0 & ~gpbCS1; 
  m_U2.writeGPIOB(m_ctrl);    // start reset
  delayMicroseconds(10);      // wait pulse width (req. =300ns) 
  m_ctrl |= gpbRES | gpbCS0 | gpbCS1;   
  m_U2.writeGPIOB(m_ctrl);    // stop reset
  delay(1);                   // wait 1ms until device is ready after reset (req. =110µs) 
  SetBrightness(4);
  SetPos(0);                  // set cursor to leftmost position
}

void HDSP2112::Begin(void) {
  m_U1.begin_SPI(m_spi_cs,m_spi_clk,m_spi_miso,m_spi_mosi,U1_addr);  // start U1
  m_U2.begin_SPI(m_spi_cs,m_spi_clk,m_spi_miso,m_spi_mosi,U2_addr);  // start U2
  m_U1.enableAddrPins();               // enable HAEN function
  m_U2.enableAddrPins();               // enable HAEN function
  for (uint8_t ic=0;ic<maxPOS;ic++) {  // set all pins to OUTPUT (normal write mode)
    m_U1.pinMode(ic, OUTPUT);
    m_U2.pinMode(ic, OUTPUT); 
  }
  Reset(); // reset both displays after SPI is working and MCP23s17 are ready
}

void HDSP2112::WrData(uint8_t addr, uint8_t data, uint8_t hid) {
  m_U1.writeGPIOA(addr);   // set address 
  m_U1.writeGPIOB(data);   // set data
  setCS(0,hid);            // cs=low
  setWR(0);                // wr=low 
  delayMicroseconds(1);    // wr active-time req.=100ns
  setWR(1);                // wr=high
  setCS(1,hid);            // cs=high
}

void HDSP2112::WrData(uint8_t addr, uint8_t data) {
  m_U1.writeGPIOA(addr);   // set address 
  m_U1.writeGPIOB(data);   // set data
  setCS(0);                // cs=low (all displays)
  setWR(0);                // wr=low 
  delayMicroseconds(1);    // wr active-time req.=100ns
  setWR(1);                // wr=high
  setCS(1);                // cs=high (all displays)
}

uint8_t HDSP2112::RdData(uint8_t addr,uint8_t hid) {
  uint8_t data=0;
  DataDirection(INPUT);    // set GPIOB to read-mode
  delayMicroseconds(1);    // wait a little
  m_U1.writeGPIOA(addr);   // set address 
  setCS(0,hid);            // cs=low
  setRD(0);                // rd=low 
  delayMicroseconds(1);    // rd-low-to-data-valid-setup-time req.=75ns 
  data=m_U1.readGPIOB();   // read data from GPIOB d[0..7]
  setRD(1);                // rd=high
  setCS(1,hid);            // cs=high
  DataDirection(OUTPUT);   // set GPIOB to normal write-mode
  delayMicroseconds(1);    // wait a little
  return data;
}


void HDSP2112::SetFlashBits(uint16_t fb) {
  for(uint16_t ic=0; ic<16; ic++) { 
    uint8_t id = (ic<8)? 0:1;      // select display [left,right]
    uint8_t addr = ic&7;           // address FLASH-Bit
    uint16_t msk = 0x8000>>ic;     // mask bit position
    uint8_t data = (uint8_t) ((msk==(msk & fb))? 1u : 0u); 
    setFL(0);                      // FL=low
    delayMicroseconds(1);          // wait a little
    WrData(addr,data,id);
    delayMicroseconds(1);          // wait a little  
    setFL(1);                      // FL=high
  }
}

void HDSP2112::SetUdcFont(const uint8_t *font, uint8_t nChars){
  for(uint8_t ic=0; ic<nChars; ic++) {
    uint32_t offset=ic*UDC_rows;
    SetUdChar(font+offset,ic);
  }
  SetUdChar(font,0); // workarround: first char needs double set
  Reset();
}

void HDSP2112::SetUdChar(const uint8_t *map, const uint8_t idx){
  WrData(adrUDA,idx,0);             // UDC address-register = current udc_char-index
  WrData(adrUDA,idx,1);             // UDC address-register = current udc_char-index
  for(uint8_t jc=0;jc<UDC_rows;jc++) { 
    uint8_t addr=adrUDR+jc;         // UDC ram address + row-address
    uint8_t data=map[jc];
    WrData(addr,data,0);
    WrData(addr,data,1);
  }
}



void HDSP2112::WriteChar(char ch) {
  if(m_pos < maxPOS) {              // check if valid position 
    uint8_t ad = adrCHR|(m_pos&7);  // select position within display (a0..a2)
    uint8_t id = (m_pos<8)? 0:1;    // select left/right hdsp2112 display
    WrData(ad,ch,id);               // draw Character to current display and position
    m_pos++;                        // inc cursor position
  }
}

void HDSP2112::WriteChar(const uint8_t pos, char ch) {
  m_pos = pos;     // update position
  WriteChar(ch);   // write char
}

size_t HDSP2112::WriteText(const uint8_t pos, const char *format, ...) {
  char loc_buf[64]={0};
  char *temp = loc_buf;
  va_list arg;
  va_list copy;
  va_start(arg, format);
  va_copy(copy, arg);
  int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
  va_end(copy);
  if(len < 0) {
    va_end(arg);
    return 0;
  }
  if(len >= (int)sizeof(loc_buf)) {  // comparation of same sign type for the compiler
    temp = (char*) malloc(len+1);
    if(temp == NULL) {
      va_end(arg);
      return 0;
    }
    len = vsnprintf(temp, len+1, format, arg);
  }
  va_end(arg);
  SetPos(pos);
  len = write((uint8_t*)temp, len);
  if(temp != loc_buf){
    free(temp);
  }
  return len;
}



uint8_t HDSP2112::Selftest(uint8_t hid) {
  m_cwr |= cwrTEST;                 // start selftest cwrTEST=1
  WrData(adrCWR,m_cwr,hid);
  delay(7000);                      // datasheet states 4.5 sec
  m_cwr &= ~cwrTEST;                // stop selftest cwrTEST=0
  WrData(adrCWR,m_cwr,hid); 
  delay(100);
  uint8_t data=RdData(adrCWR,hid);  // read back control-word-register
  delay(100);
  return ((cwrTSTOK == (data&cwrTSTOK)) ? 1 : 0); // cwrTSTOK=1 --> OK
}

uint8_t HDSP2112::UTF8_to_HDSP(uint8_t utf8_ch) {
  static uint8_t cPrev='\0'; // store previous char for extended
  uint8_t ch = '\0';         // return value 
  if( (utf8_ch<utf8Ascii)||((cPrev=='\0')&&(utf8_ch<utf8chUDC))) {  
    ch = utf8_ch;            // printable chars and user-defined-chars can be passed directly
    cPrev = '\0';            
  } else {                   // utf8 chars depends on cPrev
    switch (cPrev) {
      case 0xC2: {
        switch (utf8_ch) {
          case 0xb2: { ch=0x1d; break; } // 2 superscript
          case 0xb5: { ch=0x0c; break; } // µ
          case 0xa3: { ch=0x1e; break; } // POUND sign
          case 0xa5: { ch=0x1f; break; } // YEN sign
        }
        break; 
      }
      case 0xC3: {
        switch (utf8_ch) {
          case 0x9f: { ch=0x06; break; } // ß
          case 0x85: { ch=0x13; break; } // A-dot
          case 0xa5: { ch=0x14; break; } // a-dot
          case 0x84: { ch=0x15; break; } // Ä
          case 0xa4: { ch=0x16; break; } // ä
          case 0x96: { ch=0x17; break; } // Ö
          case 0xb6: { ch=0x18; break; } // ö
          case 0x9c: { ch=0x19; break; } // Ü
          case 0xbc: { ch=0x1a; break; } // ü
        }
        break; 
      }
      case 0xce: {
        switch (utf8_ch) {
          case 0xb1: { ch=0x05; break; } // alpha
          case 0xb2: { ch=0x06; break; } // beta
          case 0xb4: { ch=0x07; break; } // delta
          case 0x94: { ch=0x08; break; } // DELTA
          case 0xb7: { ch=0x09; break; } // eta
          case 0xb8: { ch=0x0a; break; } // theta
          case 0xbb: { ch=0x0b; break; } // lamda
          case 0xbc: { ch=0x0c; break; } // mu
          case 0x80: { ch=0x0d; break; } // pi
          case 0x83: { ch=0x0e; break; } // sigma
          case 0xa3: { ch=0x0f; break; } // SIGMA
          case 0x84: { ch=0x10; break; } // tau
          case 0xa6: { ch=0x11; break; } // PHI
          case 0xa9: { ch=0x12; break; } // OMEGA
          case 0x93: { ch=0x1c; break; } // GAMMA
        }
        break;
      }
    }
    cPrev=utf8_ch;           // store last char for utf8 handling
  }
  return(ch); // return valid char or '\0' for extended
}


// ----------------------------------------------------------------------------
// virtual functions, derived from Print class
// ----------------------------------------------------------------------------

size_t HDSP2112::write(const uint8_t *buffer, size_t size) {
  for(int32_t ic=0;ic<size;ic++) {
    write(buffer[ic]);
  }
  return size; 
}

size_t HDSP2112::write(const uint8_t character){
  uint8_t ch=UTF8_to_HDSP(character); // map to HDSP2112 alphabet
  if('\0'!=ch) {
    WriteChar(ch); // write if valid char
  }
  return 1;
}
