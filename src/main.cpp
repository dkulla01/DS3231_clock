#include <Wire.h>
#include <Adafruit_GFX.h>
#include <RTClib.h>
#include <TM1637Display.h>
#include <Arduino.h>

 // set the clock/data pins for the TM1637 display driver
#define CLK 2
#define DIO 3

// define the button pins
#define UP_BUTTON 8
#define DOWN_BUTTON 9

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
bool inSetTimeMode = false;

int upButtonState = HIGH;
bool upButtonPushed = false;
unsigned long upButtonLastPushedAt = 0;
unsigned long upButtonLastReleasedAt = 0;

int downButtonState = HIGH;
bool downButtonPushed = false;
unsigned long downButtonLastPushedAt = 0;
unsigned long downButtonLastReleasedAt = 0;

void setButtonState() {
  unsigned long now = millis();
  
  // set the button states for the up button
  upButtonState = digitalRead(UP_BUTTON);
  bool isUpButtonCurrentlyPushed = upButtonState == LOW;
  
  if (isUpButtonCurrentlyPushed && !upButtonPushed) {
    // state change: on last iteration of loop, button wasn't pushed, but now it is
    upButtonLastPushedAt = now;
  } else if (!isUpButtonCurrentlyPushed && upButtonPushed) {
    // state change: on last iteration of loop, button was pushed, but now it isn't
    upButtonLastReleasedAt = now;
  }
  upButtonPushed = isUpButtonCurrentlyPushed;

  // set the button states for the down button;
  downButtonState = digitalRead(DOWN_BUTTON);
  bool isDownButtonCurrentlyPushed = downButtonState == LOW;

  if (isDownButtonCurrentlyPushed && !downButtonPushed) {
    // state change: on last iteration of loop, button wasn't pushed, but now it is
    downButtonLastPushedAt = now;
  } else if (!isDownButtonCurrentlyPushed && downButtonPushed) {
    // state change: on last iteration of loop, button was pushed, but now it isn't
    downButtonLastReleasedAt = now;
  }
  downButtonPushed = isDownButtonCurrentlyPushed;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Clock starting!");
  display.setBrightness(7);
  display.showNumberDecEx(8888, COLON_ON);
  delay(3000); // wait for console opening
  pinMode(UP_BUTTON, INPUT);
  pinMode(DOWN_BUTTON, INPUT);
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

void runClockLoop() {
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
//todo:
// - if button pushed for the first time, increment by one.
// - if button pushed, increment has happened, but has been held for less than 500ms, don't increment
// - if button has been pushed and held for more than 2s, increment by 15
void incrementSetTime() {
  unsigned long now = millis();

  bool buttonHeld = now - upButtonLastPushedAt > 2000;
  // if the button has been held for more than 5s, go up to the next multiple of 15
  if (buttonHeld) {
    int nextLowestMultipleOf15 = minutes / 15 * 15;
    minutes += 15;
  } else  {
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

void decrementSetTime() {
  unsigned long now = millis();
  bool buttonHeld = now - downButtonLastPushedAt > 2000;

  // if the button has been held for more than 5s, go down to the next lowest multiple of 15
  if (buttonHeld) {
    int nextLowestMultipleOf15 = minutes / 15 * 15;
    
    if (nextLowestMultipleOf15 == minutes) {
      minutes -= 15;
    } else {
      minutes = nextLowestMultipleOf15;
    }
  } else {
    minutes -= 1;
  }

  if (minutes < 0) {
    minutes = 59;
    hours -= 1;
  }

  if (hours < 0) {
    hours = 23;
  }
}

void runTimeSettingLoop() {
  // floor the seconds as a matter of course
  seconds = 0;
  // if the up button is pushed increase the time
  if (upButtonPushed && !downButtonPushed) {
    incrementSetTime();
  } else if (downButtonPushed && !upButtonPushed) {
    decrementSetTime();
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

  display.showNumberDecEx(decimalDisplayTime, COLON_OFF, USE_24_HOUR_TIME); 

  delay(100);
}

void loop() {
  setButtonState();
  if (downButtonPushed && upButtonPushed) {
    Serial.println("BOTH PUSHED");
  } else if (downButtonPushed) {
    Serial.println("down button pushed");
  } else if (upButtonPushed) {
    Serial.println("up button pushed");
  }

  runTimeSettingLoop();
  // runClockLoop();
}