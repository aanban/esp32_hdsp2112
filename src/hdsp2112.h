#ifndef __HDSP2112_H__
#define __HDSP2112_H__ 

#include "MCP23S17.h"

///< configure number of displays in use 
constexpr uint8_t nDSP     = 2;              // number of displays 
constexpr uint8_t nPOS     = 8;              // number of chars per display
constexpr uint8_t maxPOS   = nPOS * nDSP;    // total number of chars

///< use the standard ESP32 SPI ports (SPI_CLK=18, SPI_MOSI=23, SPI_MISO=19)
constexpr int8_t SPI_clk   = 18;             // SPI clock 
constexpr int8_t SPI_mosi  = 23;             // SPI master-out-slave-in
constexpr int8_t SPI_miso  = 19;             // SPI master-in-slave-out

///< two mcp23s17 are connected in parallel using HAEN and addresses 1 and 7
constexpr uint8_t U1_addr  = 0b00000001;     // 0b001=0x1 --> register=0x21 
constexpr uint8_t U2_addr  = 0b00000111;     // 0b111=0x7 --> register=0x27
constexpr uint8_t PORT_A   = 0;              // Port-A = Pin[21..28]
constexpr uint8_t PORT_B   = 1;              // Port-B = Pin[ 1.. 8]

// m_ctrl = U2.PORT_B[0..7] used as hdsp2112 control signals
constexpr uint8_t gpbRES   = 0b00000001;     // U2.GPB[0] = reset
constexpr uint8_t gpbFL    = 0b00000010;     // U2.GPB[1] = flash-bit
constexpr uint8_t gpbWR    = 0b00000100;     // U2.GPB[2] = write enable
constexpr uint8_t gpbRD    = 0b00001000;     // U2.GPB[3] = read enable
constexpr uint8_t gpbCS0   = 0b00010000;     // U2.GPB[4] = cs display 0 
constexpr uint8_t gpbCS1   = 0b00100000;     // U2.GPB[5] = cs display 1
constexpr uint8_t gpbCS2   = 0b01000000;     // U2.GPB[6] = cs display 2
constexpr uint8_t gpbCS3   = 0b10000000;     // U2.GPB[7] = cs display 3

// internal hdsp2112 control-word-register flags
constexpr uint8_t cwrCLEAR = 0b10000000;     // 0=normal 1=clear flash and char
constexpr uint8_t cwrTEST  = 0b01000000;     // 0=normal 1=self test
constexpr uint8_t cwrTSTOK = 0b00100000;     // selftest result 0=failed 1=OK
constexpr uint8_t cwrBLINK = 0b00010000;     // 0=off 1=blinking
constexpr uint8_t cwrFLASH = 0b00001000;     // 0=off 1=flashing

// internal hdsp2112 base addresses
constexpr uint8_t adrUDA   = 0b00000000;     // User-Defined-Address
constexpr uint8_t adrUDR   = 0b00001000;     // User-Defined-RAM
constexpr uint8_t adrCWR   = 0b00010000;     // Control-Word-Register
constexpr uint8_t adrCHR   = 0b00011000;     // character-RAM

// ranges for ascii and extended user defined chars 
constexpr uint8_t utf8Ascii= 128;            // ascii chars 
constexpr uint8_t utf8chUDC= utf8Ascii+16;   // user defined chars

// main class is derived from Print 
class HDSP2112 : public Print {
  private:
    MCP23S17 *m_U1;     // PORT_A[0..7]=data[0..7] PORT_B[0..4]=addr[0..4]
    MCP23S17 *m_U2;     // PORT_B[0..4]=[res,fl,wr,rd,cs0,cs1]
    bool m_ok;          // 1=(U1 and U2 are valid)  0=(alloc failed)

    int8_t m_spi_cs;    // SPI chip-select
    int8_t m_spi_clk;   // SPI clock
    int8_t m_spi_mosi;  // SPI master-out-clock-in
    int8_t m_spi_miso;  // SPI master-in-clock-out

    uint8_t m_ctrl;     // control-byte [RES,FL,WR,RD,CS0,CS1,CS2,CS3]
    uint8_t m_cwr;      // internal hdsp2112 control-word-register:
                        // [CLR, selftest, blink, flash, bright(3)]
    
    uint8_t m_pos;      // current cursor position 


  public:
    // constructor
    // @param spi_cs    chip select 
    // @param spi_clk   spi-clock           (default=VSPI)
    // @param spi_mosi  master-out-slave-in (default=VSPI)
    // @param spi_miso  master-in-slave-out (default=VSPI)
    HDSP2112(const int8_t spi_cs, 
             const int8_t spi_clk  = SPI_clk, 
             const int8_t spi_mosi = SPI_mosi, 
             const int8_t spi_miso = SPI_miso
             ); 

    // destructor
    // frees the two dynamic allocated MCP23S17 instances m_U1 and m_U2
    inline ~HDSP2112() {
      if(NULL!=m_U1) {
        delete m_U1;
        m_U1=NULL; 
      }
      if(NULL!=m_U2) {
        delete m_U2; 
        m_U2=NULL; 
      } 
      m_ok=false;
    }

    // resets all hdsp2112 displays 
    // - set CS=low and RES=low 
    // - wait pulse width 10µs (min. 300ns) 
    // - set CS=high and RES=high 
    // - and wait until device is ready 1ms (min. 110µs)
    void Reset(void);

    // inits the two mcp23s17 and all hdsp2112 displays
    // - start SPI Bus
    // - init the two mcp23s17 port expander
    // - reset all hdsp2112 displays
    void Begin(void);

    // prints character ch at current cursor position: 
    // select corresponding display, print ch to current cursor position, 
    // increment cursor position
    // @param ch character to be printed
    void WriteChar(char ch);

    // prints character ch at pos:
    // select corresponding display, set cursor position to pos, print ch, 
    // increment cursor position
    // @param pos  position within display
    // @param ch   character to be printed
    void WriteChar(const uint8_t pos, char ch);

    // sets the cursor position to pos and prints text printf() like 
    // @param pos    cursor position
    // @param format printf style format string
    // @param ...    variable parameters
    size_t WriteText(const uint8_t pos, const char *format, ...);

    // starts selftest with cwrTEST=1, waits 6 sec, stops selftest with 
    // cwrTEST=0, reads back control-word-register, and checks 
    // cwrTSTOK[0=failed, 1=OK]
    // @param hid hdsp2112 identifier [0..3]
    // @return test result [0=failed, 1=OK]
    uint8_t Selftest(uint8_t hid);

    // clears all displays, i.e. fills all chars with blanks, finally 
    // sets cursor position to 0
    inline void clear(void){ 
      SetPos(0);
      for(uint8_t pos=0;pos<maxPOS;pos++){
        WriteChar(' ');
      }
      delay(20);
      SetPos(0);
    }

    // sets brightness of all displays
    // @param brightness [0..7] 0=100% 7=0%
    inline void SetBrightness(uint8_t brightness) { 
      if(m_ok) {
        uint8_t bn=brightness & 7;   // limit value to 3 bit
        for(uint8_t mc=0;mc<3;mc++){ 
          uint8_t ms=(1u<<mc);       // mask bit
          m_cwr = (ms==(bn&ms))? m_cwr|ms : m_cwr& ~ms; 
        }
        for(uint8_t hid=0;hid<nDSP; hid++) { 
          WrData(adrCWR,m_cwr,hid);  // set brightness for all displays
        }
      }
    }
    
    // sets flash bits according the given flash_bits, each bit belongs to a 
    // single char within a corresponding display. The arrangement of the 
    // flash bits: MSB=leftmost character in leftmost display MSB=rightmost 
    // character in rightmost display. 
    // @param flash bits [0=permanently on, 1=flash]
    void SetFlashBits(uint32_t flash_bits);


    // turns on/off flash mode for all displays. A previous call of 
    // SetFlashBits() determines which positions within the display will 
    // flash.
    // @param mode [0=on, 1=off]
    inline void FlashMode(uint8_t mode) {
      if(m_ok) {
        m_cwr = (0==mode) ? m_cwr & ~cwrFLASH : m_cwr|cwrFLASH;
        WrData(adrCWR,m_cwr);
      }
    }

    // turns on blink mode for all displays
    // @param mode [0=on, 1=off] 
    inline void BlinkMode(uint8_t mode) {
      if(m_ok) {
        m_cwr = (0==mode) ? m_cwr & ~cwrBLINK : m_cwr|cwrBLINK;
        WrData(adrCWR,m_cwr);
      }
    }

    // sets the cursor position, new cursor position is limited to maxPOS-1. 
    // @param pos the new cursor position 
    inline void SetPos(uint8_t pos) { m_pos=(pos<maxPOS)? pos : maxPOS-1; }

    // gets the cursor position
    // @return current cursor postion m_cur
    inline uint8_t GetPos(void) { return m_pos; }

    // translates UTF8 characters into the printable character set of the 
    // HDSP-2112 display, characters in the range [32..127] are passed 
    // directly and e.g. some extended characters like "äöü" are mapped 
    // to [0..31]
    // @param utf8_ch UTF8 character
    // @return a printable character for the 
    uint8_t UTF8_to_HDSP(uint8_t utf8_ch);

    // stores a user defined Font, the characters are filled to the 
    // user defined characters ram within all hdsp2112 displays
    // @param font user defined font with 7 rows 
    // @param nChars number of chars
    void SetUdcFont(const uint8_t *font, uint8_t nChars);

    // sets a single user defined character in the UDC Ram within all displays
    // @param map user defined character 5 cols x 7 rows
    // @param idx index in UDC-Ram
    void SetUdChar(const uint8_t *map, const uint8_t idx);


  protected:
    // ctrl signals of all displays, by writing "m_ctrl" to U2.PORT_B
    // m_ctrl = [RES,FL,WR,RD,CS0,CS1,CS2,CS3]
    inline void setCtrl(void) {
      m_U2->write8(PORT_B,m_ctrl);
    }

    // address bus of all displays, by writing "addr" to U1.PORT_A
    // @param addr address
    inline void setAddr(uint8_t addr) {
      m_U1->write8(PORT_A,addr);
    };

    // data bus of all displays, by writing "data" to U1.PORT_B 
    // @param data data 
    inline void setData(uint8_t data) {
      m_U1->write8(PORT_B,data);
    };

    // CS signal for a single hdsp2112 display with identifier hid
    // @param mod [0=CS low,1=CS high]
    // @param hid identifier [0..3], displays are arranged from left to right
    inline void setCS(uint8_t mod, uint8_t hid) {
      if(m_ok) {
        switch(hid) {
          case 0: { // left most display
            m_ctrl = (0==mod) ? m_ctrl & ~gpbCS0 : m_ctrl | gpbCS0;
            break;
          }
          case 1: { // center left display
            m_ctrl = (0==mod) ? m_ctrl & ~gpbCS1 : m_ctrl | gpbCS1;
            break;
          }
          case 2: { // center right display
            m_ctrl = (0==mod) ? m_ctrl & ~gpbCS2 : m_ctrl | gpbCS2;
            break;
          }
          case 3: { // right most display
            m_ctrl = (0==mod) ? m_ctrl & ~gpbCS3 : m_ctrl | gpbCS3;
            break;
          }
        }
        setCtrl();
      }
    }
    
    // CS signal of all hdsp2112 displays
    // @param mod [0=low,1=high]
    inline void setCS(uint8_t mod) {
      if(m_ok) {
        if(mod==0) {
          m_ctrl &= ~gpbCS0 & ~gpbCS1 & ~gpbCS2 & ~gpbCS3;
        } else {
          m_ctrl |=  gpbCS0 |  gpbCS1 |  gpbCS2 |  gpbCS3;
        }
        setCtrl();
      }
    }

    // FL signal for all hdsp2112 displays
    // @param mod [0=low,1=high]
    inline void setFL(uint8_t mod) {
      if(m_ok) {
        m_ctrl = (mod==0) ? (m_ctrl & ~gpbFL) : (m_ctrl | gpbFL);
        setCtrl();
      }
    }

    // WR signal for all hdsp2112 displays
    // @param mod [0=low,1=high]
    inline void setWR(uint8_t mod) {
      if(m_ok) {
        m_ctrl = (mod==0) ? (m_ctrl & ~gpbWR) : (m_ctrl | gpbWR);
        setCtrl();
      }
    }

    // RD signal for all hdsp2112 displays
    // @param mod [0=low,1=high]
    inline void setRD(uint8_t mod) {
      if(m_ok) {
        m_ctrl = (mod==0) ? (m_ctrl & ~gpbRD) : (m_ctrl | gpbRD);
        setCtrl();
      }
    }

    // writes data to specific hdsp2112 display with identifier hid
    // @param addr  address
    // @param data  data
    // @param hid   hdsp2112 identifier [0..3]
    void WrData(uint8_t addr, uint8_t data, uint8_t hid);

    // writes data to all hdsp2112 displays
    // @param addr  address
    // @param data  data
    void WrData(uint8_t addr, uint8_t data);

    // sets direction of MCP23s17 U1.Port-B, usual the direction is set to 
    // OUTPUT, e.g. calling a selftest, the direction of U1.PORT-B has to be 
    // set to INPUT in order to read data from the display. 
    // @param mode OUTPUT=write to display, INPUT=read from display
    inline void DataDirection(uint8_t mode) {
      if(m_ok) {
        uint8_t mask=(INPUT==mode) ? 0xff:0x00;
        m_U1->pinMode8(PORT_B,mask);
      }
    }

    // reads data from given hdsp2112 display.
    // Internally first DataDirection(INPUT) is called to set the U1.Port-B 
    // to "reading-data". After reading the data, DataDirection(OUPUT) is 
    // called to set the U1.Port-B to normal "writing-data" again. 
    // @param addr  address to write
    // @param hid   hdsp2112 identifier [0..1]
    // @return data from device
    uint8_t RdData(uint8_t addr,uint8_t hid);


    // ------------------------------------------------------------------------
    // virtual functions, derived from Print class
    // ------------------------------------------------------------------------ 

    // virtual function, used by print class, writes a single character
    // @param character single character
    // @return number of characters written
    virtual size_t write(const uint8_t character);
    
    // virtual function, used by print class, writes a  character string
    // @param buffer input character string 
    // @param size of character string
    // @return number of characters written
    virtual size_t write(const uint8_t *buffer, size_t size);

};

#endif

