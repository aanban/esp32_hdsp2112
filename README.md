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


- The two MCP23s17 are driven in parallel and with HAEN-Bit (=hardware address enable) activated, i.e. both MCP23s17 are connected to the ESP32 standard SPI bus (VSPI) and share **one single** chip-select-signal. 
The devices are selected via the **SPI control byte**  according to the pre-assigned address pins A0..A2 
(see [mcp23s17.pdf](doc/mcp23s17.pdf) , Figure 3-5: **SPI-Control-Byte Format**, page 15)


```
pre-assigned Address-Pins in this setup: 
device_0 (U1) address = 111 --> spi-control-byte = 0100111x  
device_1 (U2) address = 001 --> spi-control-byte = 0100001x  
```


| ESP32-GPIO | MCP23s17 | Comment                       |
|:----------:|:--------:|:------------------------------|
| 05         | 11       | SPI_CS,  chip-select          |
| 18         | 12       | SPI_CLK, clock                |
| 23         | 13       | SPI_MOSI, master-out-slave-in |
| 19         | 14       | SPI_MISO, master-in-slave-out |



- Two HDSP-2112 displays are driven in parallel, each is selected via its own chip-select signal

> [!NOTE]
> The recommended supply voltage is [4.5V to 5.5V]. The circuit used here operates the displays with 3.3V, which is in the abs. maximum range [-0.3V to 7.0V] of the data sheet. Even with this lower than suggested voltage, the displays work reliably and are shining bright. This has the advantage that no level shifters are required and the MCP23s17 can be connected directly to the ESP32. 


- For the flashing function, the displays were wired so that the left display generates the clock for the right display. ``Left SEL=3.3V`` , ``Right SEL=GND`` See specification [hp_HDSP-2112.pdf](doc/hp_HDSP-2112.pdf) page 9: meaning of signals *clock-select* (**CLS** pin 11) and *clock-input/output* (**CLK**  pin 12) 

![schematic](doc/mcp23s17__hdsp2112.png)

- use of [Adafruit-MCP23017-Arduino-Library](https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library) for driving the MCP23s17 
- added flashing mode
- added blinking mode
- the class **HDSP2112** is derived from the class **Print** , so the member functions like printf() can be used easily 