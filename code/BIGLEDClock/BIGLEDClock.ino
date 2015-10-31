
#include <Wire.h>                 // I2C driver library (for Chronodot)
#include <SPI.h>                  // SPI bus driver library (for OLED Display)
#include <RTClib.h>
#include <RTC_DS3231.h>           // DS3231 based Chronodot TXO real time clock driver library
#include <Adafruit_GFX.h>         // OLED display graphics library
#include <Adafruit_SSD1306.h>     // OLED display hardware driver library

#define OLED_MOSI   9             // OLED pin assignments
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 SSD1306(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);  // instantiate OLED display

RTC_DS3231 RTC;                           // instantiate Chronodot txo time base
char TimeString[20];                      // define formatted time string
char TempString[20];                      // define formatted temperature string
#define SQW_FREQ DS3231_SQW_FREQ_1024     // 0b00001000   1024Hz

byte segmentLatch = 5;  // Blue         BIG LED pin assignments
byte segmentClock = 6;  // Green     
byte segmentData = 7;   // Yellow
                        // Orange       uses 5V bus 
                        // Red          uses vin from 12v supply
                        // Black        uses gnd bus
// long digit1;            // set up BIG LED digits - must use long - 16bit tops out at 32000!     
// long digit2;
// long digit3;
// long digit4;
// long digit5;
// long digit6;
long digit1, digit2, digit3, digit4, digit5, digit6;  // set up BIG LED digits - must use long - 16bit tops out at 32000!
long hhmmss;            

void setup() {
  Serial.begin(57600);                       // serial monitor for debug messages  
  Wire.begin();                              // fire up I2C bus
  RTC.begin();                               // fire up Chronodot
    if (! RTC.isrunning()) {
      Serial.println("RTC is NOT running!");
      RTC.adjust(DateTime(__DATE__, __TIME__));
      Serial.println(__TIME__);
    }    
  SSD1306.begin(SSD1306_SWITCHCAPVCC);       // fire up OLED display

  pinMode(segmentClock, OUTPUT);
  pinMode(segmentData, OUTPUT);
  pinMode(segmentLatch, OUTPUT);
  digitalWrite(segmentClock, LOW);
  digitalWrite(segmentData, LOW);
  digitalWrite(segmentLatch, LOW);
}

// SIGNAL(TIMER0_COMPA_vect) {char c = GPS.read();}  // Read GPS port

void loop() {
  DateTime now = RTC.now();

  digit1 = now.hour()/10;               //   02 = 0   18 = 1
  digit2 = now.hour()%10;               //   02 = 2   18 = 8
  digit3 = now.minute()/10;
  digit4 = now.minute()%10;
  digit5 = now.second()/10;
  digit6 = now.second()%10;
  hhmmss = (digit1*100000)+             // lets parse out to drive the digits
           (digit2*10000)+              //    with leading zeros if necessary
           (digit3*1000)+
           (digit4*100)+
           (digit5*10)+
           (digit6*1);
  showTime(hhmmss);

  sprintf(TimeString, "%02d%02d%02d", now.hour(), now.minute(), now.second());  // format time for OLED
  
  float tempC = (RTC.getTempAsFloat());                       // pull temperature from Chronodot (in C)
  float tempF = ((tempC * 9 ) / 5) + 32;                      // convert celsius to Fahrenheit
  char tempCStr[10]; 
  dtostrf(tempC, 5, 1, tempCStr);                             // convert float to 1 dec place 
  char tempFStr[10];    
  dtostrf(tempF, 5, 1, tempFStr);                             // convert float to 1 dec place 
  char tempString[20]; 
  sprintf(TempString, "%s C | %s F", tempCStr, tempFStr);     // format temp readings for OLED

  SSD1306.setTextSize(2); 
  SSD1306.setTextColor(WHITE); 
  SSD1306.clearDisplay();
  SSD1306.setCursor(0,0);  
  SSD1306.print(TimeString);
  SSD1306.setCursor(0,20); 
  SSD1306.print(TempString);
  SSD1306.display();                                          // push print messages to OLED display
  Serial.print(now.hour(), DEC); Serial.print(":");           // debug messages to serial monitor
  Serial.print(now.minute(), DEC); Serial.print(":");
  Serial.print(now.second(), DEC); Serial.print(" | ");
  Serial.print(now.month(), DEC); Serial.print('/');
  Serial.print(now.day(), DEC); Serial.print('/');
  Serial.print(now.year(), DEC); Serial.print(" | ");
  Serial.print(tempC, DEC); Serial.print("C | ");
  Serial.print(tempF, DEC); Serial.print("F | ");
  Serial.print("Assembly: "); Serial.print(digit1); Serial.print("|");
                              Serial.print(digit2); Serial.print("|");
                              Serial.print(digit3); Serial.print("|");
                              Serial.print(digit4); Serial.print("|");
                              Serial.print(digit5); Serial.print("|");
                              Serial.print(digit6); Serial.print("|");
                              Serial.print(hhmmss); 
  Serial.println();
  delay(1000);                                        // 1 second breather
}

void showTime(unsigned long value) {
  unsigned long number = abs(value);
  for (byte x = 0 ; x < 6 ; x++) {
    int remainder = number % 10;
    postNumber(remainder, false);
    number /= 10;
  }
  digitalWrite(segmentLatch, LOW);                    // Latch the current segment data
  digitalWrite(segmentLatch, HIGH);                   // Register moves storage on rising RCK edge of
}

void postNumber(byte number, boolean decimal) {       // push digits out to BIG LEDs
#define a  1<<0                                       //       ---  a 0   mapped segments to pins
#define b  1<<6                                       //  1 f |   | b 6
#define c  1<<5                                       //       ---  g 2
#define d  1<<4                                       //  3 e |   | c 5
#define e  1<<3                                       //       ---  d 4 .DP 7
#define f  1<<1
#define g  1<<2
#define dp 1<<7
  byte segments;
  switch (number) {
    case 1: segments = b | c; break;
    case 2: segments = a | b | d | e | g; break;
    case 3: segments = a | b | c | d | g; break;
    case 4: segments = f | g | b | c; break;
    case 5: segments = a | f | g | c | d; break;
    case 6: segments = a | f | g | e | c | d; break;
    case 7: segments = a | b | c; break;
    case 8: segments = a | b | c | d | e | f | g; break;
    case 9: segments = a | b | c | d | f | g; break;
    case 0: segments = a | b | c | d | e | f; break;
    case ' ': segments = 0; break;
    case 'c': segments = g | e | d; break;
    case '-': segments = g; break;
  }
  if (decimal) segments |= dp;
  for (byte x = 0 ; x < 8 ; x++) {                        // clock these bits out to the drivers
    digitalWrite(segmentClock, LOW);
    digitalWrite(segmentData, segments & 1 << (7 - x));
    digitalWrite(segmentClock, HIGH);                     // transfer to register on rising SRCK edge 
  }
}

