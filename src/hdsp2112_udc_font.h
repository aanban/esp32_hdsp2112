#ifndef __HDSP2112_UDC_FONT_H__
#define __HDSP2112_UDC_FONT_H__

// example of user defined characters with 5 columns x 7 rows

#include <hdsp2112.h>


// arrows 
constexpr uint8_t aUp    = 141; 
constexpr uint8_t aRight = 27; 
constexpr uint8_t aDown  = 143; 
constexpr uint8_t aLeft  = 142; 

// triangles 
constexpr uint8_t tLeft  = 0; 
constexpr uint8_t tBar   = 139; 
constexpr uint8_t tRight = 138; 

// font size 5 x 7 
constexpr uint8_t UDC_nch  = 16;   // number of user defined chars
constexpr uint8_t UDC_rows = 7;    // rows per character
constexpr uint8_t UDC_size = UDC_nch * UDC_rows; // font-size 

// wipe left/right up/down, animated dot
const uint8_t UDC_wipe[UDC_size] {
  // [128..131] animated dot to square, 4 chars
  0b00000, 0b00000, 0b00000, 0b00100, 0b00000, 0b00000, 0b00000,
  0b00000, 0b00000, 0b01110, 0b01010, 0b01110, 0b00000, 0b00000,
  0b00000, 0b11111, 0b10001, 0b10001, 0b10001, 0b11111, 0b00000,
  0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000,
  // [132..138] up-->down with 7 characters
  0b11111, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000,
  0b00000, 0b11111, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000,
  0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000, 0b00000,
  0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000,
  0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000,
  0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b00000,
  0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111,
  // [139..144] left-->right with 5 characters
  0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000,
  0b01000, 0b01000, 0b01000, 0b01000, 0b01000, 0b01000, 0b01000,
  0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100,
  0b00010, 0b00010, 0b00010, 0b00010, 0b00010, 0b00010, 0b00010,
  0b00001, 0b00001, 0b00001, 0b00001, 0b00001, 0b00001, 0b00001
};

// extended font with clock, arrows and € char
const uint8_t UDC_font[UDC_size] = {
  // [128...135] clock hands, 8 characters
  0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00000, 0b00000,
  0b00001, 0b00010, 0b00100, 0b00100, 0b00000, 0b00000, 0b00000,
  0b00000, 0b00000, 0b00000, 0b00111, 0b00000, 0b00000, 0b00000,
  0b00000, 0b00000, 0b00000, 0b00100, 0b00010, 0b00001, 0b00000,
  0b00000, 0b00000, 0b00000, 0b00100, 0b00100, 0b00100, 0b00100,
  0b00000, 0b00000, 0b00000, 0b00100, 0b01000, 0b10000, 0b00000,
  0b00000, 0b00000, 0b00000, 0b11100, 0b00000, 0b00000, 0b00000,
  0b00000, 0b10000, 0b01000, 0b00100, 0b00000, 0b00000, 0b00000,
  // 136 filled circle 
  0b00000, 0b01110, 0b11111, 0b11111, 0b11110, 0b01110, 0b00000,
  // 137 heart
  0b00000, 0b01010, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000,
  // 138 triangle right
  0b01000, 0b01100, 0b01110, 0b01111, 0b01110, 0b01100, 0b01000,
  // 139 bar
  0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b00000, 0b00000,
  // 140: € sign
  0b00111, 0b01000, 0b11110, 0b01000, 0b11110, 0b01000, 0b00111,
  // 141 arrow up
  0b00100, 0b01110, 0b10101, 0b00100, 0b00100, 0b00100, 0b00100, 
  // 142 arrow left
  0b00000, 0b00100, 0b01000, 0b11111, 0b01000, 0b00100, 0b00000,
  // 143 arrow down
  0b00100, 0b00100, 0b00100, 0b00100, 0b10101, 0b01110, 0b00100
};

#endif 
//__HDSP2112_UDC_FONT_H__
