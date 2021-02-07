#include <U8g2lib.h>
#include <U8x8lib.h>

#include "fan.c" // fan bitmap

#include "src/FreqMeasure/FreqMeasure.h"

//#define DEBUGMODE

// compile time information
const char compile_date[] = __DATE__;
// version
const char swVersion[] = "1.0";

// Frequency Measurement Limits
#define MINFREQ 1     // approx 1.129 m/s or 2.5 mph
#define MAXFREQ 100   // approx 101.1 m/s or 226.2 mph

// Max speed reset timer in ms
#define RESETTIMEOUT (30*1000)

// calibration data from NRG sensor
#define SLOPE 0.759
#define OFFSET 0.37

// conversion factor for m/s to mph
#define MPHFACTOR 2.23694

// storage for max freq
float maxFreq;

// storage for max speed in mph
unsigned int maxSpeed;

// fan animation counter
unsigned char fanCount = 0;

// screen update timer
unsigned long lastUpdate = 0;

// max speed reset vars
unsigned long lastMax = 0;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

void splashScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_9x15_tr);
  u8g2.drawXBM(4,4,fan_width,fan_height,fan1_bits);
  u8g2.setCursor(40,16);
  u8g2.print("Anemometer");
  u8g2.setCursor(40,30);
  u8g2.print("by W3AXL");
  u8g2.setCursor(4,44);
  u8g2.print("Version " + String(swVersion));
  u8g2.setCursor(4,58);
  u8g2.print(compile_date);
  u8g2.sendBuffer();
  // loading animation
  for (int i=0; i<100; i+=3) {
    u8g2.drawPixel(14+(i),63);
    u8g2.drawPixel(14+(i+1),63);
    u8g2.drawPixel(14+(i+2),63);
    u8g2.sendBuffer();
  }
  delay(1000);
}

void debugScreen(float curFreq) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_9x15_tr);
  u8g2.setCursor(4,12);
  u8g2.print("Cur Freq:");
  u8g2.setCursor(16,24);
  u8g2.print(String(curFreq));
  u8g2.setCursor(4,36);
  u8g2.print("Max Freq:");
  u8g2.setCursor(16,48);
  u8g2.print(String(maxFreq));
  u8g2.setCursor(4,60);
  u8g2.print("DEBUG MODE");
  u8g2.sendBuffer();
}

void updateScreen(int curMph, int maxMph) {
  // clear screen
  u8g2.clearBuffer();
  // print current speed, with extra sauce for right-justification
  String curSpd = String(int(curMph));
  int align = 0;
  switch (curSpd.length()) {
    case 1:
      align = 54;
      break;
    case 2:
      align = 20;
      break;
  }
  u8g2.setFont(u8g2_font_logisoso54_tr);
  u8g2.setCursor(align,62);
  u8g2.print(curSpd);
  // max speed
  u8g2.setFont(u8g2_font_logisoso20_tr);
  u8g2.setCursor(88,62);
  u8g2.print(String(int(maxMph)));
  // increment the fan animation on screen update if our speed is not 0
  if (curMph > 0) {
    if (fanCount > 2) {
      fanCount = 0;
    } else {
      fanCount++;
    }
  }
  // display the proper fan graphic
  switch (fanCount) {
    case 0:
      u8g2.drawXBM(90,6,fan_width,fan_height,fan1_bits);
      break;
    case 1:
      u8g2.drawXBM(90,6,fan_width,fan_height,fan2_bits);
      break;
    case 2:
      u8g2.drawXBM(90,6,fan_width,fan_height,fan3_bits);
      break;
    case 3:
      u8g2.drawXBM(90,6,fan_width,fan_height,fan4_bits);
      break;
  }
  // send all that grafix shit
  u8g2.sendBuffer();
}

void setup() {
  // initialize frequency measurement
  FreqMeasure.begin();
  // display setup
  u8g2.begin();
  u8g2.clear();
  u8g2.setBusClock(400000);
  #ifndef DEBUGMODE
    splashScreen();
  #endif
}

void loop() {
  // update the screen if we have a new measurement available
  if (FreqMeasure.available()) {
    // get the latest measurement
    float freq = FreqMeasure.countToFrequency(FreqMeasure.read());
    // convert to mph from freq
    float rawSpeed = ((freq * SLOPE) + OFFSET) * MPHFACTOR;
    // round to nearest int by adding 0.5 before casting
    rawSpeed = rawSpeed + 0.5 - (rawSpeed<0);
    unsigned int speed = int(rawSpeed);
    // check if that's a new max
    if (speed > maxSpeed) { 
      maxSpeed = speed; 
      maxFreq = freq;
      lastMax = millis();
    } else if (millis() - lastMax > RESETTIMEOUT) {
      maxSpeed = speed;
      maxFreq = freq;
    }
    // update the screen
    #ifdef DEBUGMODE
      debugScreen(freq);
    #else
      updateScreen(speed, maxSpeed);
    #endif
    // record the screen update time
    lastUpdate = millis();
  // if we haven't gotten a frequency update in longer than the minimum measurement period, update the screen
  } else if (millis() - lastUpdate > (1000/MINFREQ)) {
    updateScreen(0, maxSpeed);
  }
}
