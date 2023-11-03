# esp32_hdsp2112
A sample ESP32 application for vintage HDSP-2112 displays, driven with MCP23s17 portexpander.

![hdsp2112_display](doc/hdsp2112_brightness.jpg) 


- The two MCP23s17 portexpander are driven in parallel via SPI and with HAEN-Bit (=hardware address enable) activated, i.e. both MCP23s17 are connected to the standard VSPI bus of the ESP32 and share one single chip-select signal. The devices are selected with the **SPI control byte** and the presetted address pins A0..A2. (see [mcp23s17.pdf](doc/mcp23s17.pdf) , Figure 3-5: **SPI-Control-Byte Format**, page 15)

```
in this setting: 
device_0(U1) address = 111 --> spi-control-byte = 0100111x  
device_1(U2) address = 001 --> spi-control-byte = 0100001x  
```

| Signal   | ESP32-GPIO | Comment               |
|:---------|:----------:|:----------------------|
| SPI_CS   | 05         |chip-select for both   |
| SPI_CLK  | 18         |SPI clock              |
| SPI_MISO | 19         |SPI master-in-slave-out|
| SPI_MOSI | 23         |SPI master-out-slave-in|


- Two HDSP-2112 displays are driven in parallel
- Contrary to the recommendation in the data sheet, the displays are operated with 3.3V instead of 5V. The advantage of this is that no additional level shifter is required. The displays work reliably with 3.3V and shine brightly. 
- For the flashing function, the displays were wired so that the left display generates the clock for the right display. See specification [hp_HDSP-2112.pdf](doc/hp_HDSP-2112.pdf) page 9: meaning of signals *clock-select* (**CLS** pin 11) and *clock-input/output* (**CLK**  pin 12) 

![schematic](doc/mcp23s17__hdsp2112.png)

- use of [Adafruit-MCP23017-Arduino-Library](https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library) for driving the MCP23s17 
- added flashing mode
- added blinking mode
- the class **HDSP2112** is derived from the class **Print** , so the member functions like printf() can be used easily 