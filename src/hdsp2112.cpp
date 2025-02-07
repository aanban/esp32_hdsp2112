#include <hdsp2112.h>
#include <hdsp2112_udc_font.h>

// ------------------------------------------------------------------------
// public members of class
// ------------------------------------------------------------------------ 

HDSP2112::HDSP2112(int8_t spi_cs, int8_t spi_clk, int8_t spi_mosi, int8_t spi_miso) {
  m_spi_cs   = spi_cs;                    // SPI chipselect
  m_spi_clk  = spi_clk;                   // SPI clock
  m_spi_mosi = spi_mosi;                  // SPI master-out-slave-in
  m_spi_miso = spi_miso;                  // SPI master-in-slave-out
  m_pos = 0;                              // set cursor to leftmost position
  m_ctrl = 0b11111111;                    // U2.GPB[0..7] = res,fl,wr,rd,cs0,cs1,x,x
  m_U1 = new MCP23S17(m_spi_cs,U1_addr);  // device U1 from schematic
  m_U2 = new MCP23S17(m_spi_cs,U2_addr);  // device U2 from schematic
  m_ok = (m_U1 && m_U2)? true: false;     // set m_ok==true if U1,U2 valid

  m_cwr = 0b00000000;     // initial hdsp2112 code-word-register
        //  |||||\_\__d2..d0 brightness = 000 = 100%
        //  ||||\_____d3     flash = 0 OFF
        //  |||\______d4     blink = 0 OFF
        //  ||\_______d5     don't care
        //  |\________d6     selftest = 0 OFF
        //  \_________d7     clear = 0 normal operation 
}

void HDSP2112::Begin(void) {
  if(m_ok) {
    SPI.begin(m_spi_clk,m_spi_miso,m_spi_mosi,m_spi_cs);
    m_U1->begin();
    m_U1->enableHardwareAddress(); // enable HAEN function
    m_U1->pinMode16(0);            // set all pins to OUTPUT
    m_U2->begin();
    m_U2->enableHardwareAddress(); // enable HAEN function
    m_U2->pinMode16(0);            // set all pins to OUTPUT
    Reset();                       // reset all displays now
  }
}

void HDSP2112::Reset(void) {
  if(m_ok) {
    m_ctrl &= ~gpbRES & ~gpbCS0 & ~gpbCS1 & ~gpbCS2 & ~gpbCS3;
    setCtrl();                 // activate reset
    delayMicroseconds(10);     // reset pulse (req. =300ns) 
    m_ctrl |= gpbRES | gpbCS0 | gpbCS1 | gpbCS2 | gpbCS2;
    setCtrl();                 // release reset
    delay(1);                  // hold (req. =110µs) 
    SetBrightness(4);          // default brightness
    SetPos(0);                 // set cursor to leftmost position
  }
}


void HDSP2112::WrData(uint8_t addr, uint8_t data, uint8_t hid) {
  if(m_ok) {
    setAddr(addr);             // set address bus
    setData(data);             // set data bus
    setCS(0,hid);              // cs=low (select display hid)
    setWR(0);                  // wr=low 
    delayMicroseconds(1);      // wr pulse (req.=100ns)
    setWR(1);                  // wr=high
    setCS(1,hid);              // cs=high (given display hid)
  }
}

void HDSP2112::WrData(uint8_t addr, uint8_t data) {
  if(m_ok) {
    setAddr(addr);             // set address 
    setData(data);             // set data
    setCS(0);                  // cs=low (all displays)
    setWR(0);                  // wr=low 
    delayMicroseconds(1);      // wr pulse (req.=100ns)
    setWR(1);                  // wr=high
    setCS(1);                  // cs=high (all displays)
  }
}

uint8_t HDSP2112::RdData(uint8_t addr, uint8_t hid) {
  uint8_t data=0;
  if(m_ok) {
    DataDirection(INPUT);      // set U1.Port-B to readmode
    delayMicroseconds(1);      // wait a little
    setAddr(addr);             // set address 
    setCS(0,hid);              // cs=low
    setRD(0);                  // rd=low 
    delayMicroseconds(1);      // data-setup-time (req.=75ns)
    data=m_U1->read8(PORT_B);  // read data from Port B d[0..7]
    setRD(1);                  // rd=high
    setCS(1,hid);              // cs=high
    DataDirection(OUTPUT);     // set U1.Port-B to writemode
    delayMicroseconds(1);      // wait a little
  }
  return data;
}

void HDSP2112::SetFlashBits(uint32_t fb) {
  if(m_ok) {
    for(uint32_t pos=0; pos<maxPOS; pos++) { 
      uint8_t hid  = pos / nPOS;      // select display
      uint8_t addr = pos % nPOS;      // select bit address
      uint32_t msk = 0x80000000>>pos; // select bit position
      uint8_t data = (uint8_t) ((msk==(msk & fb))? 1u : 0u); 
      setFL(0);                // FL=low
      delayMicroseconds(1);    // wait a little
      WrData(addr,data,hid);   // set flash bit of selected display
      delayMicroseconds(1);    // wait a little
      setFL(1);                // FL=high
    }
  }
}

void HDSP2112::SetUdcFont(const uint8_t *font, uint8_t nChars){
  if(m_ok) {
    for(uint8_t ic=0; ic<nChars; ic++) {
      uint32_t offset = ic * UDC_rows;
      SetUdChar(font + offset, ic);
    }
    SetUdChar(font,0);         // workaround: first char needs to be set twice
    Reset();                   // the display need a reset() here 
  }
}

void HDSP2112::SetUdChar(const uint8_t *map, const uint8_t idx){
  if(m_ok) {
    WrData(adrUDA,idx);        // UDC address-register = current udc_char-index
    for(uint8_t jc=0; jc<UDC_rows; jc++) { 
      uint8_t addr=adrUDR+jc;  // UDC ram address + row-address
      uint8_t data=map[jc];
      WrData(addr,data);       // write row to UDC ram
    }
  }
}

void HDSP2112::WriteChar(char ch) {
  if(m_ok) {
    if(m_pos < maxPOS) {          // check if valid position 
      uint8_t hid = m_pos / nPOS; // find display identifier
      uint8_t pos = m_pos % nPOS; // find position within display
      uint8_t adr = adrCHR|pos;   // address of position
      WrData(adr,ch,hid);         // print char to current position
      m_pos++;                    // increment cursor
    }
  }
}

void HDSP2112::WriteChar(const uint8_t pos, char ch) {
  if(m_ok) {
    m_pos = pos;    // store position to given pos
    WriteChar(ch);  // write char
  }
}

size_t HDSP2112::WriteText(const uint8_t pos, const char *format, ...) {
  if(m_ok) {
    char buf[64]={0};
    va_list args;
    va_start(args, format);
    int len=vsnprintf(buf,maxPOS+1, format,args); // limit to maxPOS
    va_end(args);
    SetPos(pos);
    len=write((uint8_t*)buf,len);
    return len; 
  }
  return 0;
}

uint8_t HDSP2112::Selftest(uint8_t hid) {
  if(m_ok) {
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
  return 0;
}

uint8_t HDSP2112::UTF8_to_HDSP(uint8_t utf8_ch) {
  uint8_t ch = '\0';           // return value 
  if(m_ok) {
    static uint8_t cPrev='\0'; // store previous char for extended
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
            case 0xbb: { ch=0x0b; break; } // lambda
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
