#include <hdsp2112.h>

// ------------------------------------------------------------------------
// public members of class
// ------------------------------------------------------------------------ 

HDSP2112::HDSP2112(int8_t spi_cs, int8_t spi_clk, int8_t spi_mosi, int8_t spi_miso) {
  m_spi_cs   = spi_cs;   // spi chipselect
  m_spi_clk  = spi_clk;  // spi clock
  m_spi_mosi = spi_mosi; // spi master-out-slave-in
  m_spi_miso = spi_miso; // spi master-in-slave-out
  m_pos = 0;             // set cursor to leftmost position
  m_cwr = 0b00000000;    // hdsp2112 code-word-register
                         // d7=clear=0 normal operation 
                         // d6=selftest=0 off  
                         // d5=x
                         // d4=blink=0
                         // d3=flash=0
                         // bright=000 = 100%
  m_ctrl = 0b11111111;   // mpc1 port-B = res,fl,wr,rd,cs0,cs1
}

void HDSP2112::Reset(void) {
  m_ctrl = m_ctrl & ~cRES & ~cCS0 & ~cCS1; 
  m_U1.writeGPIOB(m_ctrl);    // reset + both cs activated
  delayMicroseconds(10);      // wait pulse width (req. =300ns) 
  m_ctrl |= cRES|cCS0|cCS1;   
  m_U1.writeGPIOB(m_ctrl);    // reset + both cs deactivated
  delay(1);                   // wait 1ms until device is ready after reset (req. =110µs) 
  SetPos(0);                  // set cursor to leftmost position
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
}

void HDSP2112::WrData(uint8_t id, uint8_t addr, uint8_t data) {
  m_U2.writeGPIOA(addr);   // set address 
  m_U2.writeGPIOB(data);   // set data
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
    m_U2.writeGPIOA(addr);       // set address 
    m_U2.writeGPIOB(data);       // set data
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
    uint8_t id = (m_pos<8)? 0:1;   // select hdsp2112 display
    WrData(id,ad,ch);              // draw Character to current display-position
    m_pos++;                       // inc cursor position
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
// ------------------------------------------------------------------------

size_t HDSP2112::write(const uint8_t *buffer, size_t size){
  for(int32_t ic=0; ic<size; ic++) {
    write(buffer[ic]);
  } 
  return size;
}

size_t HDSP2112::write(const uint8_t character){
  WriteChar(character);
  return 1;
}
