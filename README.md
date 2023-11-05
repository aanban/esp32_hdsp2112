# esp32_hdsp2112
A sample ESP32 application for two vintage HDSP-2112 displays, driven with two MCP23s17 16-bit I/O expander.
Tested with VSCode + platformio and a 38pin ESP32 NodeMCU board.

```ini
[env:esp32dev]
platform  = espressif32
board     = esp32dev
framework = arduino
```

![hdsp2112_display](doc/hdsp2112_brightness.jpg) 


## 1. schematic

![schematic](doc/mcp23s17__hdsp2112.png)

### 1.1. wiring of the two MCP23s17 
---
It is possible to connect multiple MCP23s17s to a SPI bus in parallel with a single CS signal only. Steps to do:   
#### 1.1.1 define address pins
The address pins A0..A2 of the MCP23s17 must be wired so that each module has a unique address, e.g. the pre-assigned address-pins in this setup are:

| device | A0 | A1 | A2 | SPI-control | Comment                    |
|:------:|:--:|:--:|:--:|:-----------:|:---------------------------|
| U1     |  1 |  1 |  1 | 01000111x   | GPA=d[0..7], GPB=a[0..4]   |
| U2     |  0 |  0 |  1 | 01000001x   | GPB=[res,fl,wr,rd,cs0,cs1] |


#### 1.1.2. activate hardware-address-enable 
Additionally, when initializing the two MCP23s17, the `HAEN` bit (=hardware address enable) must be set within the `IOCON` register (=I/O expander configuration), i.e. call member function `enableAddrPins()`
```c
  m_U1.enableAddrPins();  // enable HAEN function
  m_U2.enableAddrPins();  // enable HAEN function
```
#### 1.1.3. select devices via SPI-control-byte
The devices are selected via the **SPI control byte**  according to the pre-assigned address pins A0..A2 
(see [mcp23s17.pdf](doc/mcp23s17.pdf) , Figure 3-5: **SPI-Control-Byte Format**, page 15) This is done internally, the address is set while calling the `begin_SPI()` member function, that is starting the SPI module. 

```c  
  m_U1.begin_SPI(m_spi_cs,m_spi_clk,m_spi_miso,m_spi_mosi,U1_addr);
  m_U2.begin_SPI(m_spi_cs,m_spi_clk,m_spi_miso,m_spi_mosi,U2_addr);
```

The table below shows the wiring of the SPI connections for the EPS32 and the MCP23s17

| ESP32-GPIO | MCP23s17 U1 | MCP23s17 U2 | Comment                       |
|:----------:|:-----------:|:-----------:|:------------------------------|
| 05         | 11          | 11          | SPI_CS,  chip-select          |
| 18         | 12          | 12          | SPI_CLK, clock                |
| 23         | 13          | 13          | SPI_MOSI, master-out-slave-in |
| 19         | 14          | 14          | SPI_MISO, master-in-slave-out |
|            | 15 A0 3.3v  | 15 A0 3.3v  | MCP23s17-address              |
|            | 16 A1 3.3v  | 16 A1 GND   |                               |
|            | 17 A2 3.3v  | 17 A2 GND   |                               |


### 1.2. wiring of the HDSP-2112
---
Two HDSP-2112 displays are connected in parallel, each is selected via its own chip-select signal

---
> [!NOTE]
> The recommended supply voltage is [4.5V to 5.5V]. The circuit used here operates the displays with 3.3V, which remains within the abs. maximum range [-0.3V to 7.0V] of the data sheet. Even with this lower than suggested voltage, the displays work reliably and are shining bright. Using 3.3V has the advantage that no level shifters are required and the MCP23s17 can be connected directly to the ESP32. 
---

For a proper flashing function, the displays were wired so that the left display generates the clock for the right display. ``Left SEL=3.3V`` , ``Right SEL=GND`` See specification [hp_HDSP-2112.pdf](doc/hp_HDSP-2112.pdf) page 9:  *clock-select* (**CLS** pin 11) and *clock-input/output* (**CLK**  pin 12) 

## 2. SW hints
### 2.1. MCP23s17 Library
The [Adafruit-MCP23017-Arduino-Library](https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library) is used to control the MCP23s17 port expander

### 2.2 parent classes
The class `HDSP2112` is derived from the class `Print` , so the member functions like printf() can be used easily  

### 2.3 useful member functions
some member functions as follows
 - `Reset()` reset the displays (via hardware signal)
 - `Clear()` fill display with blanks
 -  `SetFlashBits()` and `FlashMode()` to flash  individual characters of the displays
 - `BlinkMode()` to let the whole display blink
 - `SetBrightness()` to adjust the brightness
 - `Selftest()` perform a selftest 
 - `WriteChar()` print a given character
 - `printf()` standard printf for printing strings
  
   
---
> [!NOTE]
>The Selftest() function itself works, however reading the status-bit and showing `OK` or `Failed` is on the 
todo-list. Reading means: change the `m_U2.pinMode()` from OUTPUT to INPUT and set RD signal for the MCP23s17 accordingly
