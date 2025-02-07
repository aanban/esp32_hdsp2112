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


# 1. Schematic

![schematic](doc/mcp23s17__hdsp2112.png)

## 1.1. Wiring of multiple MCP23s17 
The data sheet states that it is possible to connect multiple MCP23s17s to a SPI bus in parallel with one single CS signal only. Follow these steps:
### 1.1.1. Pre-assign address pins A0..A2
Each MCP23s17 module requires a unique address, that is done with the wiring of the address pins `A[0..2]`. The pre-assigned address pins in this setup are:

| device | A0 | A1 | A2 | SPI-control | Comment                            |
|:------:|:--:|:--:|:--:|:-----------:|:-----------------------------------|
| U1     |  0 |  0 |  1 | 01000001x   | GPA=d[0..7], GPB=a[0..4]           |
| U2     |  1 |  1 |  1 | 01000111x   | GPB=[res, fl, wr, rd, cs0, cs1, cs2, cs3] |


### 1.1.2. Activate HAEN bit (hardware-address-enable) in IOCON register
When initializing the two MCP23s17, the `HAEN` bit (=hardware address enable) must be set within the `IOCON` register (=I/O expander configuration). That is done by calling the member function `enableAddrPins()`
```cpp
  m_U1.enableAddrPins();  // enable HAEN function
  m_U2.enableAddrPins();  // enable HAEN function
```
### 1.1.3. Select the MCP23s17 devices via SPI-control-byte
The devices are selected via the **SPI control byte**  according to the pre-assigned address pins `A0..A2`

> see [mcp23s17.pdf](doc/mcp23s17.pdf) , Figure 3-5: **SPI-Control-Byte Format**, page 15 

This is automatically handled within the driver. The specific addresses are set during initialization of the SPI-bus within the `begin_SPI()` member function.

```cpp  
  m_U1.begin_SPI(m_spi_cs,m_spi_clk,m_spi_miso,m_spi_mosi,U1_addr);
  m_U2.begin_SPI(m_spi_cs,m_spi_clk,m_spi_miso,m_spi_mosi,U2_addr);
```

## 1.2. Wiring ESP32 SPI bus

The table below shows the wiring of the SPI connections for the ESP32 and the MCP23s17, i.e. the standard SPI bus of the ESP32 (VSPI) 

| ESP32-GPIO | MCP23s17 U1 | MCP23s17 U2 | Comment                            |
|:----------:|:-----------:|:-----------:|:-----------------------------------|
| 05         | 11          | 11          | SPI_CS,  VSPI chip-select          |
| 18         | 12          | 12          | SPI_CLK, VSPI clock                |
| 23         | 13          | 13          | SPI_MOSI, VSPI master-out-slave-in |
| 19         | 14          | 14          | SPI_MISO, VSPI master-in-slave-out |


## 1.3. Wiring the HDSP-2112 displays
The HDSP-2112 displays are connected in parallel (`address-bus`, `data-bus`, `RES`, `FL`, `WR`, `RD`). Each is selected via its own chip-select signal (`CS_0`, `CS_1`, `CS_2`, `CS_3` )

To get a proper flashing function, the displays were wired (clock select `SEL` pin 11) so that the left display generates the clock `FLASH_CLK` for the other displays on the right. 

| SEL (Pin 11) | level | comment                                   |
|:------------:|:-----:|:------------------------------------------|
| U3           | 3.3V  |CLK (pin 12) work as output of flash-clock |
| U4           | GND   |CLK (pin 12) work as input                 |
| U5           | GND   |CLK (pin 12) work as input                 |
| U6           | GND   |CLK (pin 12) work as input                 |


>See specification [hp_HDSP-2112.pdf](doc/hp_HDSP-2112.pdf) page 9:  *clock-select* (**CLS** pin 11) and *clock-input/output* (**CLK**  pin 12) 


> [!NOTE]
> The recommended supply voltage is [4.5V to 5.5V]. The circuit used here, operates the displays with 3.3V, which remains within the abs. maximum range [-0.3V to 7.0V] of the data sheet. Even with 3.3V the displays work reliably and are shining bright. Using 3.3V has the advantage that the MCP23s17 can be connected directly to the ESP32, i.e. no level shifters are required.



# 2. Considerations about SW
## 2.1. MCP23s17 Library
The [MCP23S17](https://github.com/RobTillaart/MCP23S17) library is used to control the MCP23s17 port expander

## 2.2. Parent Class
The class `HDSP2112` is derived from the class `Print` , so the member functions like `printf()` can be used easily. 

## 2.3. Useful member functions
Some member functions as follows
 - `Reset()` resets the displays (via hardware signal)
 - `Clear()` fills display with blanks
 -  `SetFlashBits()` and `FlashMode()` used to flash individual characters of the displays
 - `BlinkMode()` lets the whole display blink
 - `SetBrightness()` used to adjust the brightness
 - `Selftest()` performs a selftest 
 - `WriteChar()` prints a given character
 - `printf()` works link standard printf for printing strings
 - `SetUdcFont()` set 16 user defined chars 

## 2.4. utf8 handling
Some utf8 characters can be mapped to the internal character map, that is handled by the member function UTF8_to_HDSP(), e.g. `Ä` is mapped to `0x15` or `alpha` is mapped to `0x05`

## 2.5. Selftest() function
The HDSP2112 self test is activated by setting `cwrTEST=1` within the control-word-register. The datasheet states that the procedure needs about 4 sec, I set the wait time to 7sec. Afterwards the `cwrTSTOK` bit of the control-word-register indicates the status of the test. `cwrTSTOK=1` means that the test was `OK` else it `Failed`. Reading from the HDSP2121 means: change the MCP23s17's `m_U1.pinMode()` of the from OUTPUT to INPUT and set RD signal for the displays accordingly.

> [!NOTE]
> During the tests I found out that a Reset() must be called after the Selftest() function, in order to avoid a strange behavior of the BlinkMode and FlashMode functions. 

## 2.6. Basic example
The following main.cpp shows a basic example:

```cpp
#include "hdsp2112.h"
HDSP2112 d;    // create default instance

void setup() {
  d.Begin();   // int SPI, MCP23s17 and HDSP2112 displays
  d.printf("Hdsp2112-Display"); // print something to display
  delay(2000);
}

void loop() {
}

```
