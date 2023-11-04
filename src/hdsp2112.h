#ifndef __HDSP2112_H__
#define __HDSP2112_H__ 

#ifndef __ADAFRUIT_MCP23X17_H__
#include <Adafruit_MCP23X17.h>
#endif

// use the standard ESP32 SPI ports (SPI_CLK=18, SPI_MOSI=23, SPI_MISO=19)
constexpr int8_t SPI_cs   =  5;        ///< SPI chip select 
constexpr int8_t SPI_clk  = 18;        ///< SPI clock 
constexpr int8_t SPI_mosi = 23;        ///< SPI master-out-slave-in
constexpr int8_t SPI_miso = 19;        ///< SPI master-in-slave-out

// two mcp23s17 devices are connected in parallel and are accessed via addresses a[2..0]
constexpr uint8_t MCP_a0 = 0b001;      ///< a2=0 a1=0 a0=1 0b001 --> 0x21
constexpr uint8_t MCP_a1 = 0b111;      ///< a2=1 a1=1 a0=1 0b111 --> 0x27

// m_mcp1.port-B works a control port for the two hdsp2112 displays
constexpr uint8_t cRES = 0b00000001;   ///< GPB0(1) = reset
constexpr uint8_t cFL  = 0b00000010;   ///< GPB1(2) = flash-bit
constexpr uint8_t cWR  = 0b00000100;   ///< GPB2(3) = write enable
constexpr uint8_t cRD  = 0b00001000;   ///< GPB3(4) = read enable
constexpr uint8_t cCS0 = 0b00010000;   ///< GPB4(5) = chip select 0
constexpr uint8_t cCS1 = 0b00100000;   ///< GPB5(6) = chip select 1

// dhsp2112 control-word-register
constexpr uint8_t cwrClear    = 0b10000000;   ///< 0=normal 1=clear flash and char
constexpr uint8_t cwrSelftest = 0b01000000;   ///< 0=normal 1=self test
constexpr uint8_t cwrBlink    = 0b00010000;   ///< 0=off 1=blinking
constexpr uint8_t cwrFlash    = 0b00001000;   ///< 0=off 1=flashing

// hdsp2112 address-range for internal RAMs: Flash,User-defined,Control and Character-RAM
constexpr uint8_t adrFLR=0b00011000;   ///< hdsp2112 Flash-RAM address
constexpr uint8_t adrUDA=0b00100000;   ///< hdsp2112 User-Defined-Address address
constexpr uint8_t adrUDR=0b00101000;   ///< hdsp2112 User-Defined-RAM address
constexpr uint8_t adrCWR=0b00110000;   ///< hdsp2112 Control-Word-Register address
constexpr uint8_t adrCHR=0b00111000;   ///< hdsp2112 character-RAM address

constexpr uint8_t maxPOS  = 16;        ///< two hdsp2112 displays with 8 chars each

enum class mod_e { low=0, high=1 };    ///< logic level of signals

class HDSP2112 : public Print {
  private:
    Adafruit_MCP23X17 m_mcp0; ///< mcs23s17 (MCP_a0) GPA[0..7]=data[0..7] GBB[0..4]=address[0..4]
    Adafruit_MCP23X17 m_mcp1; ///< mcs23s17 (MCP_A1) GBB[0..4]=[res,fl,wr,rd,cs0,cs1]
    int8_t m_spi_cs;          ///< SPI chip-select
    int8_t m_spi_clk;         ///< SPI clock
    int8_t m_spi_mosi;        ///< SPI master-out-clock-in
    int8_t m_spi_miso;        ///< SPI master-in-clock-out
    uint8_t m_ctrl;           ///< hdsp2112 control byte --> GBB[0..4]: [RES,FL,WE,CS0,CS1]
    uint8_t m_cwr;            ///< hdsp2112 control-word-register [clear, selftest, blink, flash, bright(3)]
    uint8_t m_pos;            ///< current cursor position 
    

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

    // prints a character to the current position m_pos and corresponding display
    // - select left or right display
    // - print char to m_pos
    // - increment m_pos
    // @oaram ch charcter to be printed
    void WriteChar(char ch);

    // start selftest
    // - set selftest bit in controll-word-register
    // @param id 0=left 1=right display
    void Selftest(uint8_t id);

    // clear Display
    inline void clear(void){ 
      SetPos(0); 
      printf("                "); 
      SetPos(0);
    }

    // set the brightness for left and right display
    // extract brightness bits[0..2] and fill bit[0..2] of m_cwr register
    // @param brightness [0..7] 0=100% 7=0%
    inline void SetBrightness(uint8_t brightness) { 
      uint8_t bn=brightness & 7;   // limit to bit[0..2]
      for(uint8_t mc=0;mc<3;mc++){ // extract and fill bit[0..2]
        uint8_t ms=(1u<<mc);       // mask bit
        m_cwr = (ms==(bn&ms))? m_cwr|ms : m_cwr& ~ms; // set or clear bit within m_cwr
      }
      WrData(0, adrCWR, m_cwr); // set control register of left display
      WrData(1, adrCWR, m_cwr); // set control register of right display
    }
    
    // set Flash Bits
    // @param flashbits of single char posistions of left and right display
    void SetFlashBits(uint16_t flashbits);

    // set flash-Mode
    // @param mode [0=on, 1=off]
    inline void FlashMode(uint8_t mode) {
      m_cwr = (0==mode) ? m_cwr & ~cwrFlash : m_cwr|cwrFlash;
      WrData(0, adrCWR,m_cwr);
      WrData(1, adrCWR,m_cwr);
    }

    // set blink-Mode
    // @param mode [0=on, 1=off]
    inline void BlinkMode(uint8_t mode) {
      m_cwr = (0==mode) ? m_cwr & ~cwrBlink : m_cwr|cwrBlink;
      WrData(0, adrCWR,m_cwr);
      WrData(1, adrCWR,m_cwr);
    }

    // set the cursor postion
    // @param pos the new cursor position 
    inline void SetPos(uint8_t pos) { m_pos=(pos<maxPOS)? pos : maxPOS-1; }

    // gets the cursor postion
    // @return current cursor postion m_cur
    inline uint8_t GetPos(void) { return m_pos; }

  protected:

    // sets chip_select input of given hdsp2112 display to [low,high]
    // @param id   hdsp2112 identifier [0..1]
    // @param mod [low,high]
    inline void setCS(uint8_t id, mod_e mod) {
      if(mod==mod_e::low) {
        m_ctrl &= (0==id)? ~cCS0 : ~cCS1;
      } else {
        m_ctrl |= (0==id)? cCS0 : cCS1;
      }
      m_mcp1.writeGPIOB(m_ctrl);
    }

    // sets fl input of given hdsp2112 display to [low,high]
    // @param mod [low,high]
    inline void setFL(mod_e mod) {
      m_ctrl = (mod==mod_e::low) ? (m_ctrl & ~cFL) : (m_ctrl | cFL);
      m_mcp1.writeGPIOB(m_ctrl);
    }

    // control write_enable input of all hdsp2112 displays
    // @param mod [low,high]
    inline void setWR(mod_e mod) {
      m_ctrl = (mod==mod_e::low) ? (m_ctrl & ~cWR) : (m_ctrl | cWR);
      m_mcp1.writeGPIOB(m_ctrl);
    }

    // write data to given hdsp2112 display
    // @param id    hdsp2112 identifier [0..1]
    // @param addr  address to write
    // @param data  data to write
    void WrData(uint8_t id, uint8_t addr, uint8_t data);



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

