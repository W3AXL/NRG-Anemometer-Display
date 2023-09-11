#include <U8g2lib.h>
#include <U8x8lib.h>

#include "fan3d.c" // fan bitmap

#include "src/FreqMeasure/FreqMeasure.h"

//#define DEBUGMODE

// compile time information
const char compile_date[] = __DATE__;
// version
const char swVersion[] = "1.1";

// Frequency Measurement Limits
#define MINFREQ 1     // approx 1.129 m/s or 2.5 mph
#define MAXFREQ 100   // approx 101.1 m/s or 226.2 mph

// Max speed reset timer in ms
#define RESETTIMEOUT (30*1000)

// Serial port report interval
#define REPORT_INTERVAL 2000

// Idle fan animation framerate
#define IDLE_ANIM_TIME 2000

// calibration data from NRG sensor
#define SLOPE 0.759
#define OFFSET 0.37

// conversion factor for m/s to mph
#define MPHFACTOR 2.23694

// storage for max freq
float maxFreq;

// storage for current speed in mph
char curSpeed;

// storage for max speed in mph
char maxSpeed;

// fan animation counter
unsigned char fanCount = 0;

// screen update timer
unsigned long lastUpdate = 0;

// serial report timer
unsigned long lastReport = 0;

// fan idle timeout
unsigned long lastIdle = 0;

// max speed reset vars
unsigned long lastMax = 0;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

void splashScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_9x15_tr);
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
    switch (i%4) {
      case 0:
        u8g2.drawXBM(4,12,fan_width,fan_height,fan1_bits);
        break;
      case 1:
        u8g2.drawXBM(4,12,fan_width,fan_height,fan2_bits);
        break;
      case 2:
        u8g2.drawXBM(4,12,fan_width,fan_height,fan3_bits);
        break;
      case 3:
        u8g2.drawXBM(4,12,fan_width,fan_height,fan4_bits);
        break;
    }
  }
  delay(2000);
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
  u8g2.setFont(u8g2_font_logisoso54_tn);
  switch (curSpd.length()) {
    // single digit
    case 1:
      u8g2.setCursor(54,62);
      u8g2.print(curSpd);
      break;
    // two digits
    case 2:
      u8g2.setCursor(54,62);
      u8g2.print(curSpd[1]);
      u8g2.setCursor(24,62);
      u8g2.print(curSpd[0]);
      break;
    // three digits
    case 3:
      u8g2.setCursor(54,62);
      u8g2.print(curSpd[2]);
      u8g2.setCursor(24,62);
      u8g2.print(curSpd[1]);
      u8g2.setCursor(-3,62);
      u8g2.print(curSpd[0]);
      break;
  }
  // max speed
  u8g2.setFont(u8g2_font_logisoso20_tn);
  u8g2.setCursor(88,62);
  u8g2.print(String(int(maxMph)));
  // MPH text
  u8g2.setFont(u8g2_font_profont17_tr);
  u8g2.setCursor(90,37);
  u8g2.print("MPH");
  // Fan animation
  if (curMph > 0) {
    // increment the fan animation on screen update if our speed is not 0
    if (fanCount > 2) {
      fanCount = 0;
    } else {
      fanCount++;
    }
  } else {
    // Idle animation if speed is zero
    if ((millis() - lastIdle) > IDLE_ANIM_TIME) {
      lastIdle = millis();
      // switch between frames 0 and 1
      if (fanCount) {
        fanCount = 0;
      } else {
        fanCount = 1;
      }
    }
  }
  // display the proper fan graphic
  switch (fanCount) {
    case 0:
      u8g2.drawXBM(90,8,fan_width,fan_height,fan1_bits);
      break;
    case 1:
      u8g2.drawXBM(90,8,fan_width,fan_height,fan2_bits);
      break;
    case 2:
      u8g2.drawXBM(90,8,fan_width,fan_height,fan3_bits);
      break;
    case 3:
      u8g2.drawXBM(90,8,fan_width,fan_height,fan4_bits);
      break;
  }
  // send all that grafix shit
  u8g2.sendBuffer();
}

void setup() {
  // initialize frequency measurement
  FreqMeasure.begin();
  // Serial port init
  Serial.begin(115200);
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
    curSpeed = int(rawSpeed);
    // check if that's a new max
    if (curSpeed > maxSpeed) { 
      maxSpeed = curSpeed; 
      maxFreq = freq;
      lastMax = millis();
    } else if (millis() - lastMax > RESETTIMEOUT) {
      maxSpeed = curSpeed;
      maxFreq = freq;
      lastMax = millis();
    }
    // update the screen
    #ifdef DEBUGMODE
      debugScreen(freq);
    #else
      updateScreen(curSpeed, maxSpeed);
    #endif
    // record the screen update time
    lastUpdate = millis();
  // if we haven't gotten a frequency update in longer than the minimum measurement period, update the screen
  } else if (millis() - lastUpdate > (1000/MINFREQ)) {
    // also reset our max if it's time
    if (millis() - lastMax > RESETTIMEOUT) {
      maxSpeed = 0;
      maxFreq = 0;
      lastMax = millis();
    }
    updateScreen(0, maxSpeed);
  }
  // Send serial report update if we can
  if (Serial.availableForWrite() > 16) {
    if (millis() - lastReport > REPORT_INTERVAL) {
      char buf[16];
      sprintf(buf, "CUR:%u,MAX:%u", curSpeed, maxSpeed);
      Serial.println(buf);
      lastReport = millis();
    }
  }
}
