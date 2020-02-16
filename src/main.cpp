#include <Wire.h>
#include <Adafruit_GFX.h>
#include <RTClib.h>
#include <TM1637Display.h>
#include <Arduino.h>

 // set the clock/data pins for the TM1637 display driver
#define CLK 2
#define DIO 3

bool USE_24_HOUR_TIME = false;
uint8_t COLON_ON = 0x40;
uint8_t COLON_OFF = 0x00;

// RTC_DS3231 rtc;
TM1637Display display(CLK, DIO);

RTC_DS3231 rtc;


int hours = 0;
int minutes = 0;
int seconds = 0;

bool blinkColon = false;


void setup() {
    Serial.begin(115200);
  Serial.println("Clock starting!");
  display.setBrightness(7);
  display.showNumberDecEx(8888, COLON_ON);
  delay(3000); // wait for console opening

  rtc.begin();

  if (!rtc.begin()) {
    Serial.println("Unable to find RTC");
    while (true) {
      Serial.println("uh oh this is bad");
      // blink "8888" on the clock to indicate a problem
      display.showNumberDecEx(8888, COLON_ON);
      delay(500);
      display.clear();
      delay(500);
    }
  }

  if (rtc.lostPower()) {
    Serial.println("lost power, let's set the clock time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }


}

void loop() {
    if (minutes == 0) {
    // Get the time from the DS3231.
    DateTime now = rtc.now();
    // Print out the time for debug purposes:
    Serial.print("Read date & time from DS3231: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    // Now set the hours and minutes.
    hours = now.hour();
    minutes = now.minute();
  }

  int displayHours = hours;
  if (!USE_24_HOUR_TIME) {
    if (hours > 12) {
      displayHours = hours - 12;
    } else if (hours == 0) {
      displayHours = 12;
    }
  }
  int decimalDisplayTime = displayHours * 100 + minutes;
  blinkColon = !blinkColon;
  uint8_t colonMask = blinkColon
    ? COLON_ON
    : COLON_OFF;
  display.showNumberDecEx(decimalDisplayTime, colonMask, USE_24_HOUR_TIME);
  delay(1000);
  seconds += 1;
  if (seconds > 59) {
    seconds = 0;
    minutes += 1;
  }
  
  if (minutes > 59) {
    minutes = 0;
    hours += 1;
  }
  
  if (hours > 23) {
    hours = 0;
  }
}