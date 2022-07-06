#ifndef W_MAX_7219_H
#define W_MAX_7219_H

#include "MaxFont.h"
#include "WPin.h"

#define DELAY_CLEARING 10
#define DELAY_SCROLLING 30
#define DELAY_OVERSIZE 4444
#define max7219_reg_noop 0x00
#define max7219_reg_digit0 0x01
#define max7219_reg_digit1 0x02
#define max7219_reg_digit2 0x03
#define max7219_reg_digit3 0x04
#define max7219_reg_digit4 0x05
#define max7219_reg_digit5 0x06
#define max7219_reg_digit6 0x07
#define max7219_reg_digit7 0x08
#define max7219_reg_decodeMode 0x09
#define max7219_reg_intensity 0x0a
#define max7219_reg_scanLimit 0x0b
#define max7219_reg_shutdown 0x0c
#define max7219_reg_displayTest 0x0f

class WMax7219 : public WPin {
 public:
  WMax7219(int dinPin, int csPin, int clkPin, int numSegments)
      : WPin(dinPin, NO_MODE) {
    this->csPin = csPin;
    this->clkPin = clkPin;
    this->numSegments = numSegments;
    this->font = defaultFont;
    this->rotate = true;
    this->animate = true;
    this->buffer = new byte[numSegments * 8];
    this->reset();
    this->init();
  }

  void init() {
    pinMode(this->getPin(), OUTPUT);
    pinMode(clkPin, OUTPUT);
    pinMode(csPin, OUTPUT);
    digitalWrite(clkPin, HIGH);
    setCommand(max7219_reg_scanLimit, 0x07);
    // using an led matrix (not digits)
    setCommand(max7219_reg_decodeMode, 0x00);
    // not in shutdown mode
    setCommand(max7219_reg_shutdown, 0x01);
    // no display test
    setCommand(max7219_reg_displayTest, 0x00);
    // empty registers, turn all LEDs off
    clear();
    // the first 0x0f is the value you can set
    setIntensity(0x0f / 2);
  }

  void clear() {
    this->reset();
    this->reload();
  }

  void setCommand(byte command, byte value) {
    digitalWrite(csPin, LOW);
    for (int i = 0; i < numSegments; i++) {
      shiftOut(this->getPin(), clkPin, MSBFIRST, command);
      shiftOut(this->getPin(), clkPin, MSBFIRST, value);
    }
    digitalWrite(csPin, LOW);
    digitalWrite(csPin, HIGH);
  }

  void setIntensity(byte intensity) {
    setCommand(max7219_reg_intensity, intensity);
  }

  byte writeSprite(int x, const byte* sprite) {
    byte w = sprite[0];
    for (int i = 0; i < w; i++) {
      this->setColumn(i + x, sprite[i + 1]);
    }
    reload();
    return w;
  }

  void scrollUp() {
    for (int i = 0; i < numSegments * 8; i++) {
      byte cValue = getColumn(i);
      for (int b = 0; b < 7; b++) {
        bool bs = bitRead(cValue, b + 1);
        if (bs) {
          bitSet(cValue, b);
        } else {
          bitClear(cValue, b);
        }
      }
      bitClear(cValue, 7);
      setColumn(i, cValue);
    }
  }

  enum WLedState { FITS, OVERSIZE, SCROLL_LEFT, SCROLLED_LEFT, SCROLL_RIGHT };

  void showText(String text) {
    this->text = utf8ascii(text);
    this->textWidth = this->getWidth(this->text);
    int x = max(0, numSegments * 8 / 2 - textWidth / 2);
    if (animate) {
      for (int y = 8; y > 0; y--) {
        this->scrollUp();
        this->writeString(x, y, this->text);
        this->reload();
        delay(DELAY_CLEARING);
      }
    }
    this->writeString(x, 0, this->text);
    reload();
    // Prepare scrolling
    this->state = (numSegments * 8 >= textWidth ? FITS : OVERSIZE);
    startTime = 0;
  }

  void loop(unsigned long now) {
    if (startTime == 0) {
      startTime = now;
    }
    if ((this->state == OVERSIZE) && (now - startTime >= DELAY_OVERSIZE)) {
      // Start SCROLLING
      this->state = SCROLL_LEFT;
      this->stateCounter = 0;
      startTime = now;
    }
    if ((this->state == SCROLL_LEFT) && (now - startTime >= DELAY_SCROLLING)) {
      this->stateCounter--;
      startTime = now;
      this->writeString(this->stateCounter, 0, this->text);
      reload();
      if (this->stateCounter == (numSegments * 8 - this->textWidth)) {
        this->state = SCROLLED_LEFT;
      }
    }
    if ((this->state == SCROLLED_LEFT) && (now - startTime >= DELAY_OVERSIZE)) {
      // Start SCROLLING
      this->state = SCROLL_RIGHT;
      this->stateCounter = (numSegments * 8 - this->textWidth);
      startTime = now;
    }
    if ((this->state == SCROLL_RIGHT) && (now - startTime >= DELAY_SCROLLING)) {
      this->stateCounter++;
      startTime = now;
      this->writeString(this->stateCounter, 0, this->text);
      reload();
      if (this->stateCounter == 0) {
        this->state = OVERSIZE;
      }
    }
  }

 protected:
 private:
  WLedState state;
  int stateCounter;
  unsigned long startTime;
  char c1;
  String text;
  int textWidth;

  byte csPin;
  byte clkPin;
  byte numSegments;
  byte* buffer;
  byte* font;
  bool rotate;
  bool animate;

  int getFontIndex(char c) {
    int offset = 0;
    for (int i = 0; i < c; i++) {
      offset += pgm_read_byte(font + offset);
      offset++;  // skip size byte we used above
    }
    return (offset);
  }

  byte writeChar(int x, int y, byte c) {
    int fIndex = this->getFontIndex(c);
    byte w = font[fIndex];
    for (int i = 0; (i < w) && ((i + x) < (numSegments * 8)); i++) {
      if ((i + x) >= 0) {
        byte fValue = font[fIndex + i + 1];
        if (y != 0) {
          fValue = getColumn(i + x) | (fValue << y);
        }
        this->setColumn(i + x, fValue);
      }
    }
    return w;
  }

  int writeString(int x, int y, String s) {
    if (y == 0) this->clearBuffer();
    for (int i = 0; (i < s.length()) /*&& (x < numSegments * 8)*/; i++) {
      // Add Space between characters
      x += (i > 0);
      x += writeChar(x, y, s.charAt(i));
    }
    return x;
  }

  int getWidth(String s) {
    int w = 0;
    for (int i = 0; i < s.length(); i++) {
      w += (i > 0);
      w += font[this->getFontIndex(s.charAt(i))];
    }
    return w;
  }

  void clearBuffer() {
    for (int i = 0; i < (numSegments * 8); i++) {
      buffer[i] = 0;
    }
  }

  void reset() {
    this->clearBuffer();
    this->text = "";
    this->textWidth = 0;
    this->state = FITS;
    this->stateCounter = 0;
    this->startTime = 0;
  }

  void reload() {
    for (int i = 0; i < 8; i++) {
      int col = i;
      digitalWrite(csPin, LOW);
      for (int j = 0; j < numSegments; j++) {
        shiftOut(this->getPin(), clkPin, MSBFIRST, i + 1);
        shiftOut(this->getPin(), clkPin, MSBFIRST, buffer[col]);
        col += 8;
      }
      digitalWrite(csPin, LOW);
      digitalWrite(csPin, HIGH);
    }
  }

  void setColumn(int x, byte value) {
    if (rotate) {
      byte n = x / 8;
      byte bufferBit = 7 - x % 8;
      for (int b = 0; b < 8; b++) {
        bool bs = bitRead(value, b);
        if (bs) {
          bitSet(buffer[b + (n * 8)], bufferBit);
        } else {
          bitClear(buffer[b + (n * 8)], bufferBit);
        }
      }
    } else {
      buffer[x] = value;
    }
  }

  byte getColumn(int x) {
    if (rotate) {
      byte result = 0b00000000;
      byte n = x / 8;
      byte bufferBit = 7 - x % 8;
      for (int b = 0; b < 8; b++) {
        bool bs = bitRead(buffer[b + (n * 8)], bufferBit);
        if (bs) bitSet(result, b);
      }
      return result;
    } else {
      return buffer[x];
    }
  }

  void setRotate(bool rotate) {
    this->rotate = rotate;
    this->clear();
  }

  byte utf8ascii(byte ascii) {
    if (ascii < 128) {
      // Standard ASCII-set 0..0x7F handling
      c1 = 0;
      return ascii;
    }
    // get previous input
    byte last = c1;  // get last char
    c1 = ascii;      // remember actual character
    switch (last) {
      // conversion depending on first UTF8-character
      case 0xC2:
        return ascii;
        break;
      case 0xC3:
        return (ascii | 0xC0);
        break;
      case 0x82:
        if (ascii == 0xAC) return 0x80;  // special case Euro-symbol
    }
    return 0;  // otherwise: return zero, if character has to be ignored
  }

  String utf8ascii(String s) {
    String result = "";
    c1 = 0;
    char c;
    for (int i = 0; i < s.length(); i++) {
      c = utf8ascii(s.charAt(i));
      if (c != 0) {
        result = result + c;
      }
    }
    return result;
  }
};

#endif
