#ifndef __HDSP2112_H__
#define __HDSP2112_H__ 

#ifndef __ADAFRUIT_MCP23X17_H__
#include <Adafruit_MCP23X17.h>
#endif

// use the standard ESP32 SPI ports (SPI_CLK=18, SPI_MOSI=23, SPI_MISO=19)
constexpr int8_t SPI_cs    =  5;          // SPI chip select 
constexpr int8_t SPI_clk   = 18;          // SPI clock 
constexpr int8_t SPI_mosi  = 23;          // SPI master-out-slave-in
constexpr int8_t SPI_miso  = 19;          // SPI master-in-slave-out

// two mcp23s17 devices are connected in parallel and are accessed via addresses a[2..0]
constexpr uint8_t U1_addr  = 0b00000001;  // a[2]=1 a[1]=1 a[0]=1 0b001 --> 0x21
constexpr uint8_t U2_addr  = 0b00000111;  // a[2]=0 a[1]=0 a[0]=1 0b111 --> 0x27

// m_U2.GPB[0..5] control signals hdsp2112 displays
constexpr uint8_t gpbRES   = 0b00000001;  // (pin 1) GPB[0] = reset
constexpr uint8_t gpbFL    = 0b00000010;  // (pin 2) GPB[1] = flash-bit
constexpr uint8_t gpbWR    = 0b00000100;  // (pin 3) GPB[2] = write enable
constexpr uint8_t gpbRD    = 0b00001000;  // (pin 4) GPB[3] = read enable
constexpr uint8_t gpbCS0   = 0b00010000;  // (pin 5) GPB[4] = chip select 0
constexpr uint8_t gpbCS1   = 0b00100000;  // (pin 6) GPB[5] = chip select 1

// hdsp2112 control-word-register
constexpr uint8_t cwrCLEAR = 0b10000000;  // 0=normal 1=clear flash and char
constexpr uint8_t cwrTEST  = 0b01000000;  // 0=normal 1=self test
constexpr uint8_t cwrTSTOK = 0b00100000;  // selftest result 0=failed 1=OK
constexpr uint8_t cwrBLINK = 0b00010000;  // 0=off 1=blinking
constexpr uint8_t cwrFLASH = 0b00001000;  // 0=off 1=flashing

// hdsp2112 address for Flash,User-defined,Control and Character-RAM
constexpr uint8_t adrUDA   = 0b00000000;  // hdsp2112 User-Defined-Address address
constexpr uint8_t adrUDR   = 0b00001000;  // hdsp2112 User-Defined-RAM address
constexpr uint8_t adrCWR   = 0b00010000;  // hdsp2112 Control-Word-Register address
constexpr uint8_t adrCHR   = 0b00011000;  // hdsp2112 character-RAM address

constexpr uint8_t maxPOS   = 16;          // two hdsp2112 displays with 8 chars each

constexpr uint8_t utf8Ascii=128;          // ascii chars 
constexpr uint8_t utf8chUDC=utf8Ascii+16; // user defined chars

enum class mod_e { low=0, high=1 };       // logic level of signals

class HDSP2112 : public Print {
  private:
    Adafruit_MCP23X17 m_U1;   // mcs23s17 (U1_addr) GPA[0..7]=data[0..7] GBB[0..4]=address[0..4]
    Adafruit_MCP23X17 m_U2;   // mcs23s17 (U2_addr) GBB[0..4]=[res,fl,wr,rd,cs0,cs1]
    int8_t m_spi_cs;          // SPI chip-select
    int8_t m_spi_clk;         // SPI clock
    int8_t m_spi_mosi;        // SPI master-out-clock-in
    int8_t m_spi_miso;        // SPI master-in-clock-out
    uint8_t m_ctrl;           // hdsp2112 control byte --> GBB[0..4]: [RES,FL,WE,CS0,CS1]
    uint8_t m_cwr;            // hdsp2112 control-word-register [clear, selftest, blink, flash, bright(3)]
    uint8_t m_pos;            // current cursor position 

  public:
    // constructor
    // @param spi_cs spi chip select 
    // @param spi_clk spi clock
    // @param spi_mosi spi master-out-slave-in
    // @param spi_miso spi master-in-slave-out
    HDSP2112(const int8_t spi_cs=SPI_cs, 
             const int8_t spi_clk=SPI_clk, 
             const int8_t spi_mosi=SPI_mosi, 
             const int8_t spi_miso=SPI_miso); 

    // hardware reset for both hdsp2112 displays 
    // - set CS=low and RES=low 
    // - wait pulsewith 10µs (min. 300ns) 
    // - set CS=high and RES=high 
    // - and wait until device is ready 1ms (min. 110µs)
    void Reset(void);

    // init mcp23s17 portexpander and hdsp2112 displays
    // - start SPI Bus
    // - init the two mcp23s17 portexpander
    // - reset the two hdsp2112 displays
    void Begin(void);

    // prints a character, 
    // select left or right display, 
    // print char to m_pos, 
    // increment m_pos
    // @param ch charcter to be printed
    void WriteChar(char ch);

    // start selftest cwrTEST=1, wait 6 sec, stop selftest cwrTEST=0, 
    // read back controll-word-register, and check cwrTSTOK[0=failed, 1=OK]
    // @param display id [0=left, 1=right]
    // @return test result [0=failed, 1=OK]
    uint8_t Selftest(uint8_t id);

    // clear Display, fill all chars with blanks
    inline void clear(void){ 
      SetPos(0); 
      printf("                "); 
      SetPos(0);
      delay(20);
    }

    // set brightness for both displays, fill brightness[0..2] into m_cwr[0..2] register
    // @param brightness [0..7] 0=100% 7=0%
    inline void SetBrightness(uint8_t brightness) { 
      uint8_t bn=brightness & 7;   // limit to bit[0..2]
      for(uint8_t mc=0;mc<3;mc++){ // extract and fill bit[0..2]
        uint8_t ms=(1u<<mc);       // mask bit
        m_cwr = (ms==(bn&ms))? m_cwr|ms : m_cwr& ~ms; 
      }
      WrData(adrCWR,m_cwr,0);      // left display
      WrData(adrCWR,m_cwr,1);      // right display
    }
    
    // set Flash-Bits, each bit belongs to one char posision of left and 
    // right display 
    // @param flashbits [1=flash, 0=permanently on]
    void SetFlashBits(uint16_t flashbits);

    // set flash-Mode, the Flash-Bits have to be set before, 
    // they determine which char pos will flash
    // @param mode [0=on, 1=off]
    inline void FlashMode(uint8_t mode) {
      m_cwr = (0==mode) ? m_cwr & ~cwrFLASH : m_cwr|cwrFLASH;
      WrData(adrCWR,m_cwr);
    }

    // set blink-Mode, the complete display will blink
    // @param mode [0=on, 1=off]
    inline void BlinkMode(uint8_t mode) {
      m_cwr = (0==mode) ? m_cwr & ~cwrBLINK : m_cwr|cwrBLINK;
      WrData(adrCWR,m_cwr);
    }

    // set the cursor postion
    // @param pos the new cursor position 
    inline void SetPos(uint8_t pos) { m_pos=(pos<maxPOS)? pos : maxPOS-1; }

    // gets the cursor postion
    // @return current cursor postion m_cur
    inline uint8_t GetPos(void) { return m_pos; }

    // translates UTF8 characters into the printable character 
    // set of the HDSP-2112 display, characters in the range [32..127] 
    // are passed directly and e.g. some extended characters like äöü 
    // are mapped to [0..31]
    // @param utf8_ch UTF8 character
    // @return a printable character for the 
    uint8_t UTF8_to_HDSP(uint8_t utf8_ch);

    // set a user defined Font, the charactes are filled to the user defined characater ram
    // @param font user defined font with 7 rows 
    // @param nChars number of chars
    void SetUdcFont(const uint8_t *font, uint8_t nChars);

    // set a user defined character in the UDC Ram
    // @param map user defined character 5 cols x 7 rows
    // @param idx index in UDC-Ram
    void SetUdChar(const uint8_t *map, const uint8_t idx);

  protected:

    // sets chip_select input of given hdsp2112 display to [low,high]
    // @param id  hdsp2112 identifier [0=left, 1=right]
    // @param mod [low,high]
    inline void setCS(uint8_t id, mod_e mod) {
      if(mod==mod_e::low) {
        m_ctrl &= (0==id)? ~gpbCS0 : ~gpbCS1;
      } else {
        m_ctrl |= (0==id)?  gpbCS0 :  gpbCS1;
      }
      m_U2.writeGPIOB(m_ctrl);
    }

    // sets chip_select input of both hdsp2112 displays to [low,high]
    // @param mod [low,high]
    inline void setCS(mod_e mod) {
      if(mod==mod_e::low) {
        m_ctrl &= ~gpbCS0 & ~gpbCS1;
      } else {
        m_ctrl |=  gpbCS0 |  gpbCS1;
      }
      m_U2.writeGPIOB(m_ctrl);
    }

    // sets fl input of both displays to [low,high]
    // @param mod [low,high]
    inline void setFL(mod_e mod) {
      m_ctrl = (mod==mod_e::low) ? (m_ctrl & ~gpbFL) : (m_ctrl | gpbFL);
      m_U2.writeGPIOB(m_ctrl);
    }

    // control write_enable input of both displays
    // @param mod [low,high]
    inline void setWR(mod_e mod) {
      m_ctrl = (mod==mod_e::low) ? (m_ctrl & ~gpbWR) : (m_ctrl | gpbWR);
      m_U2.writeGPIOB(m_ctrl);
    }

    // control read_enable input of both displays
    // @param mod [low,high]
    inline void setRD(mod_e mod) {
      m_ctrl = (mod==mod_e::low) ? (m_ctrl & ~gpbRD) : (m_ctrl | gpbRD);
      m_U2.writeGPIOB(m_ctrl);
    }

    // Writes data to given hdsp2112 display
    // @param id    hdsp2112 identifier [0=left,1=right]
    // @param addr  address to write
    // @param data  data to write
    void WrData(uint8_t addr, uint8_t data, uint8_t id);

    // Writes data to both hdsp2112 display
    // @param addr  address to write
    // @param data  data to write
    void WrData(uint8_t addr, uint8_t data);

    // set direction of the MCP23s17 U1 GDB-port.
    // it is needed for reading data d[0..7] a display 
    // [OUTPUT=writing to display, INPUT=reading from display]
    // @param mode [OUTPUT,INPUT]
    inline void DataDirection(uint8_t mode){
      for(uint8_t ic=8;ic<16;ic++ ) {
        m_U1.pinMode(ic,mode);
      }
    }

    // Reads data from given hdsp2112 display.
    // Internally first DataDirection(INPUT) is called to set the MCP23s17 to "read-mode" 
    // and after reading the data, DataDirection(OUPUT) is called to set the MCP23s17 to normal "write-mode" again. 
    // @param id    hdsp2112 identifier [0..1]
    // @param addr  address to write
    // @return data from device
    uint8_t RdData(uint8_t id, uint8_t addr);


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

