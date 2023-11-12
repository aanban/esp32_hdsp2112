#include <hdsp2112.h>

// ------------------------------------------------------------------------
// public members of class
// ------------------------------------------------------------------------ 

HDSP2112::HDSP2112(int8_t spi_cs, int8_t spi_clk, int8_t spi_mosi, int8_t spi_miso) {
  m_spi_cs   = spi_cs;       // spi chipselect
  m_spi_clk  = spi_clk;      // spi clock
  m_spi_mosi = spi_mosi;     // spi master-out-slave-in
  m_spi_miso = spi_miso;     // spi master-in-slave-out
  m_pos = 0;                 // set cursor to leftmost position
  m_cwr = 0b00000000;        // hdsp2112 code-word-register
                             // d7=clear=0 normal operation 
                             // d6=selftest=0 off  
                             // d5=x
                             // d4=blink=0
                             // d3=flash=0
                             // bright=000 = 100%
  m_ctrl = 0b11111111;       // mpc1 port-B = res,fl,wr,rd,cs0,cs1
  m_scroll_mode = 0;         // scroll off
  m_srcoll_ts_pre= 0;        // scroll timestamp
  memset(m_txt,0,maxTXT);    // init text with 0
}

void HDSP2112::Reset(void) {
  m_ctrl = m_ctrl & ~cRES & ~cCS0 & ~cCS1; 
  m_U2.writeGPIOB(m_ctrl);    // reset + both cs activated
  delayMicroseconds(10);      // wait pulse width (req. =300ns) 
  m_ctrl |= cRES|cCS0|cCS1;   
  m_U2.writeGPIOB(m_ctrl);    // reset + both cs deactivated
  delay(1);                   // wait 1ms until device is ready after reset (req. =110µs) 
  SetPos(0);                  // set cursor to leftmost position
  m_scroll_mode=0;            // no scrolling
}

void HDSP2112::Begin(void) {
  // first init the two mcp23s17 and enable HEAN function
  m_U1.begin_SPI(m_spi_cs,m_spi_clk,m_spi_miso,m_spi_mosi,U1_addr);  // start U1
  m_U2.begin_SPI(m_spi_cs,m_spi_clk,m_spi_miso,m_spi_mosi,U2_addr);  // start U2
  m_U1.enableAddrPins();               // enable HAEN function
  m_U2.enableAddrPins();               // enable HAEN function
  for (uint8_t ic=0; ic<16; ic++) {    // set all pins to OUPUT
    m_U1.pinMode(ic, OUTPUT);
    m_U2.pinMode(ic, OUTPUT); 
  }
  // second do Reset on both hdsp2112 displays now
  Reset(); 
  m_srcoll_ts_pre=millis();            // init scrolling time stamp
}

void HDSP2112::WrData(uint8_t id, uint8_t addr, uint8_t data) {
  m_U1.writeGPIOA(addr);   // set address 
  m_U1.writeGPIOB(data);   // set data
  setCS(id,mod_e::low);    // cs=low
  setWR(mod_e::low);       // wr=low 
  delayMicroseconds(1);    // wr active-time req. = 100ns
  setWR(mod_e::high);      // wr=high
  setCS(id,mod_e::high);   // cs=high
}


// flashbits MSB=leftmost character MSB=rightmost character 
void HDSP2112::SetFlashBits(uint16_t fb) {
  for(uint16_t ic=0; ic<16; ic++) { 
    uint8_t id = (ic<8)? 0:1;    // select display [left,right]
    uint8_t addr = ic&7;         // address of FLASH-Bit
    uint16_t msk = 0x8000>>ic;   // bit pos mask
    uint8_t data = (uint8_t) ((msk==(msk & fb))? 1u : 0u); 
    setFL(mod_e::low);           // FL=low
    m_U1.writeGPIOA(addr);       // set address 
    m_U1.writeGPIOB(data);       // set data
    setCS(id,mod_e::low);        // cs=low
    setWR(mod_e::low);           // wr=low 
    delayMicroseconds(1);        // wr active-time req. = 100ns
    setWR(mod_e::high);          // wr=high
    setCS(id,mod_e::high);       // cs=high
    setFL(mod_e::high);          // FL=high
  }
}

void HDSP2112::WriteChar(char ch) {
  if(m_pos < maxPOS) {             // check if valid position 
    uint8_t ad = adrCHR|(m_pos&7); // select position within display (a0..a2)
    uint8_t id = (m_pos<8)? 0:1;   // select left/right hdsp2112 display
    WrData(id,ad,ch);              // draw Character to current display and position
    m_pos++;                       // inc cursor position
  }
}

void HDSP2112::ScrollText(void) {
  int32_t ic=0, end=_min((m_pos_txt+maxPOS), m_len_txt);
  SetPos(0);       // jumpt to left posistion
  for(ic=m_pos_txt; ic<end; ic++) {
    WriteChar(m_txt[ic]);
  }
  m_pos_txt++;     // prepare next Scroll
  if(m_pos_txt > m_len_txt - maxPOS) {
    m_pos_txt=0;   // text ist complete, start over from beginning
  }
}

void HDSP2112::Loop(void) {
  if(1==m_scroll_mode) { 
    uint32_t ts_new=millis(); 
    if((ts_new-m_srcoll_ts_pre) > m_srcoll_period ) {
      ScrollText();  // it's time to scroll 
      m_srcoll_ts_pre=ts_new;
    }
  }
}

void HDSP2112::Selftest(uint8_t id) {
  m_cwr |= cwrSelftest;      // selftest bit activ
  WrData(id, adrCWR, m_cwr);
  delay(6000);
  m_cwr &= ~cwrSelftest;     // selftest bit inactiv
  WrData(id, adrCWR, m_cwr); 
}

// ----------------------------------------------------------------------------
// virtual functions, derived from Print class
// ----------------------------------------------------------------------------

size_t HDSP2112::write(const uint8_t *buffer, size_t size) {
  if(0==m_scroll_mode) { // write directly to display
    for(uint32_t ic=0;ic<size && ic<maxPOS;ic++) {
      write(buffer[ic]);
    }
  } else { // scrolling mode: store buffer with leading/trailing blanks for scrolling
    int32_t ic=0, jc=0, j_max=maxTXT-maxPOS-1;  // limit 
    for(ic=0,jc=0; ic<maxPOS; ic++,jc++) {
      m_txt[jc]=' ';  // add leading blanks
    }
    for(ic=0; ic<size && jc<j_max; ic++, jc++) {
      m_txt[jc]=buffer[ic]; // store text to m_txt after leading blanks
    } 
    for(ic=0; ic<maxPOS && jc<maxTXT-1; ic++, jc++) {
      m_txt[jc]=' ';  // add trailing blanks 
    }
    m_len_txt=jc;     // set string length
    m_pos_txt=0;      // reset text pos index
    ScrollText();     // start scrolling
  }
  return size;
}

size_t HDSP2112::write(const uint8_t character){
  WriteChar(character);
  return 1;
}
