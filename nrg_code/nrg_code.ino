#include <U8g2lib.h>
#include <U8x8lib.h>

#include "fan.c" // fan bitmap

//#define DEBUGMODE

// compile time information
const char compile_date[] = __DATE__;
// version
const char swVersion[] = "0.2";

#define PULSEPIN A5

#define THRESH 128

#define AVGSMP 5 // current value moving average samples

#define MAXPERIOD 5000000 // max valid period in uS
#define MINPERIOD 5000    // min valid period in uS

#define MINSPEED 2 // minimum valid speed in mph
#define MAXSPEED 250 // max valid speed in mph

#define OUTLIER_MULTIPLIER 4 // the current running average multipled and divided by this term defines the bounds of the outlier detection algorithm

#define INTERVAL 1000

#define SCREENINTERVAL 375

// calibration data
#define SLOPE 0.759
#define OFFSET 0.37

// timing measurement variables
bool trig = false;
unsigned long prevTime = 0;
unsigned long newTime = 0;

// real-time moving average storage
unsigned long avgArray[AVGSMP] = {MAXPERIOD};
int avgIndex = 0;
unsigned long lastAvgTime = 0;

// storage for max speed (min period) (start at max period)
unsigned long lowPeriod = MAXPERIOD;

// screen refresh timer
unsigned long lastScreen = 0;

// fan animation counter
unsigned char fanCount = 0;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

/**
 *  initAvg()
 *  
 *  initializes the average array with the default, lowest period reading
 */
void initAvg() {
    for (int i=0;i<AVGSMP;i++) {
        avgArray[i] = MAXPERIOD;
    }
}

/**
 *  getAvg()
 * 
 *  calculates and returns the average calculated from the running average array
 */
unsigned long getAvg() {
    unsigned long avgTotal = 0;
    for (int i=0;i<AVGSMP;i++) {
        avgTotal += avgArray[i];
    }
    return avgTotal / AVGSMP;
}

/**
 *  appendAvg(unsigned long period)
 * 
 *  adds a new reading to the average array
 *  also performs outlier detection
 * 
 *  inputs:
 *      - period:   the new period to append
 */
void appendAvg(unsigned long period) {
    // get the average up to this point
    unsigned long oldAvg = getAvg();
    // get the limits
    unsigned long minAllowed = oldAvg / OUTLIER_MULTIPLIER;
    unsigned long maxAllowed = oldAvg * OUTLIER_MULTIPLIER;
    // here's where we do the outlier detection
    if (period < minAllowed) {
        avgArray[avgIndex] = minAllowed;    // below the min period (too fast), we append the min period
    } else if (period > maxAllowed) {
        avgArray[avgIndex] = maxAllowed;    // above the max period (too slow), we add the max period
    } else {
        avgArray[avgIndex] = period;        // if we're within bounds, we just add the measured period
    }
    // increment the running average index
    if (avgIndex >= (AVGSMP - 1)) {
        avgIndex = 0;
    } else {
        avgIndex++;
    }
}

/**
 *  splashScreen()
 * 
 *  draws the startup splash screen
 */
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

/**
 *  periodToSpeed(unsigned long period)
 *  
 *  calculates and returns a speed in MPH from a period in uS
 *  inputs: 
 *      - period:   the measured period in uS
 */
float periodToSpeed(unsigned long period) {
    // divide by zero protection
    if (period < 1) {period = 1;}
    // convert uS to frequency in Hz
    float freq = 1 / (float(period) / 1000000);
    // use parameters to get m/s
    float mps = ( (float)SLOPE * (float)freq ) + (float)OFFSET;
    // convert to mph
    float mph = mps * 2.23694;
    if (mph < MINSPEED) { mph = 0; }
    if (mph > MAXSPEED) { mph = MAXSPEED; }
    return mph;
}

/**
 *  debugScreen()
 * 
 *  draws a debug screen on the OLED, used for troubleshooting
 */
void debugScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_9x15_tr);
    u8g2.setCursor(4,12);
    u8g2.print("Cur Period:");
    u8g2.setCursor(16,24);
    u8g2.print(String(getAvg()));
    u8g2.setCursor(4,36);
    u8g2.print("Min Period:");
    u8g2.setCursor(16,48);
    u8g2.print(String(lowPeriod));
    u8g2.setCursor(4,60);
    u8g2.print("DEBUG MODE");
    u8g2.sendBuffer();
}

/**
 *  updateScreen()
 * 
 *  routine for drawing the main screen during normal operation
 */
void updateScreen() {
    // calculate speeds from periods
    float curMph = periodToSpeed(getAvg());
    float maxMph = periodToSpeed(lowPeriod);
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
    // initialize average avgArrray
    //initAvg();
    //pinmode 
    //pinMode(PULSEPIN, INPUT_PULLUP);
    // display setup
    u8g2.begin();
    u8g2.clear();
    u8g2.setBusClock(400000);
    #ifdef DEBUGMODE
        debugScreen();
    #else
        splashScreen();
        updateScreen();
    #endif
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
        newTime = micros();
        // calculate that period and check if it's out of range
        unsigned long period = newTime - prevTime;
        if (period > MAXPERIOD) { period = MAXPERIOD; }
        // add it to the current averages
        appendAvg(period);
        lastAvgTime = millis();
        // check if it's a new max low
        if (getAvg() < lowPeriod) {lowPeriod = getAvg();}
    } else if (!pin && trig) {
        // if we were triggered but are now low, reset everything
        trig = false;
        // if we've waited longer than the max period, add a max period to the average
        if (micros() - newTime > MAXPERIOD) {
            lastAvgTime = millis();
            appendAvg(MAXPERIOD);
        }
    } else if (!pin && !trig) {
        // if nothing is happening but we're past the max timeout, add another 0 to the moving average
        if (millis() - lastAvgTime > INTERVAL) {
            lastAvgTime = millis();
            appendAvg(MAXPERIOD);
        }
    }
    // screen update timer
    if (millis() - lastScreen > SCREENINTERVAL) {
        lastScreen = millis();
        #ifdef DEBUGMODE
        debugScreen();
        #else
        updateScreen();
        #endif
    }
}
