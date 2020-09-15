#include <MovingAverage.h>

#include <U8g2lib.h>
#include <U8x8lib.h>

#include "fan.c" // fan bitmap

// compile time information
const char compile_date[] = __DATE__;
// version
const char swVersion[] = "0.1";

#define PULSEPIN A5

#define THRESH 128

#define CURAVGSMP 4 // current value moving average samples

#define MAXPERIOD 5000 // max valid period

#define MINSPEED 2 // minimum valid speed in mph
#define MAXSPEED 250 // max valid speed in mph

#define INTERVAL 1000

#define SCREENINTERVAL 500

// calibration data
#define SLOPE 0.759
#define OFFSET 0.37

// timing measurement variables
bool trig = false;
unsigned long prevTime = 0;
unsigned long newTime = 0;

// real-time moving average storage
MovingAverage<unsigned> curAvg(CURAVGSMP, MAXPERIOD);
unsigned long lastCurAvg = 0;

// storage for max speed (min period)
unsigned int minPeriod = MAXPERIOD;

// screen refresh timer
unsigned long lastScreen = 0;

// fan animation counter
unsigned char fanCount = 0;

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
  for (int i=0; i<100; i+=2) {
    u8g2.drawPixel(14+(i),63);
    u8g2.drawPixel(14+(i+1),63);
    u8g2.sendBuffer();
  }
  delay(1000);
}

float periodToSpeed(unsigned int period) {
  // divide by zero protection
  if (period < 1) {period = 1;}
  // convert to frequency in Hz
  float freq = 1 / (float(period) / 1000);
  // use parameters to get m/s
  float mps = (0.759 * freq) + 0.37;
  // convert to mph
  float mph = mps * 2.23694;
  if (mph < MINSPEED) { mph = 0; }
  if (mph > MAXSPEED) { mph = MAXSPEED; }
  return mph;
}

void updateScreen() {
  // calculate speeds from periods
  float curMph = periodToSpeed(curAvg.get());
  float maxMph = periodToSpeed(minPeriod);
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
  // fan bitmap animation plays if we're moving
  if (curMph > MINSPEED) {
    if (fanCount > 2) {
      fanCount = 0;
    } else {
      fanCount++;
    }
  }
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
  //pinmode 
  //pinMode(PULSEPIN, INPUT_PULLUP);
  // display setup
  u8g2.begin();
  u8g2.clear();
  splashScreen();
  updateScreen();
}

void loop() {
  // read the pin
  int pinRdg = analogRead(PULSEPIN);
  // it's high if it's above the threshold
  bool pin = (pinRdg > THRESH);
  // now logic
  if (pin && !trig) {
    // if we're high and we haven't triggered yet, start a measurement
    trig = true;
    // store the previous time and record the new one
    prevTime = newTime;
    newTime = millis();
    // calculate that period and check if it's out of range
    int period = newTime - prevTime;
    if (period > MAXPERIOD) { period = MAXPERIOD; }
    // add it to the current averages
    curAvg.push(period);
    lastCurAvg = millis();
    // check if it's a new max low
    if (curAvg.get() < minPeriod) {minPeriod = curAvg.get();}
  } else if (!pin && trig) {
    // if we were triggered but are now low, reset everything
    trig = false;
    // if we've waited longer than the max period, add a max period to the average
    if (millis() - newTime > MAXPERIOD) {
      lastCurAvg = millis();
      curAvg.push(MAXPERIOD);
    }
  } else if (!pin && !trig) {
    // if nothing is happening but we're past the max timeout, add another 0 to the moving average
    if (millis() - lastCurAvg > INTERVAL) {
      lastCurAvg = millis();
      curAvg.push(MAXPERIOD);
    }
  }
  // screen update timer
  if (millis() - lastScreen > SCREENINTERVAL) {
    lastScreen = millis();
    updateScreen();
  }
}
