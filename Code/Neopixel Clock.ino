/*
*ARDUINO NEOPIXEL DIGITAL CLOCK
*     _     _     _     _
*    |_|   |_| . |_|   |_|
*    |_|   |_| . |_|   |_|
*
*WS2812B 5V NIOPIXEL ADDRESSABLE LED STRIP
*DS3231 RTC TIME MODULE
*IR REMOTE CONTROL
*PIEZO BUZZER
*
*/

#include<FastLED.h>
#include <Wire.h>
#include <DS3231.h>
#include <IRremote.h>

/********************** PINs DECLARATION **********************/

#define NUM_LEDS 58       //Number of LEDS
#define LED_PIN 2         //LED pin
#define LEDS_PER_SEG 2    //Number of LEDs on each segment
#define IRpin 4           //IR sensor pin
#define Piezo 3

/******************* IR Recever Match Code ********************/

// Enter your IR Remote key code 
#define BrightUP    0xF9067F80  //0xC00058
#define BrightDN  0xFA057F80  //0xC00059
#define Mode            0xE51A7F80  //0xC0005C
#define NextColor       0xFC037F80  //0xC0005B
#define PrevColor       0xFD027F80  //0xC0005A
#define Play            0xFE017F80
#define EQ              0xFB047F80

/************************ VARIABLES **************************/

uint8_t bright = 100;   //Default Brightness Level
uint8_t hue = 0;      
uint8_t ONleds = 0;

int OLEDS[58];
CRGB Leds[NUM_LEDS];
DS3231 clock;
RTCDateTime dt;

unsigned long lastCode;     //Previous Remote key Pressed
unsigned long current = 0;
int mode = 0;

// SEVEN SEGMENT DIGIT OFFSET LED VALUES
int digits[] = { 0, (7 * LEDS_PER_SEG), (7 * LEDS_PER_SEG * 2 + 2), (7 * LEDS_PER_SEG * 3 + 2) };

// SEVEN SEGMENT DISPLAY DEFINITIONS
bool seg[14][7] {
  {false, true,   true,   true,   true,   true,   true},    //ZERO
  {false, false,  false,  true,   true,   false,  false},   //ONE
  {true,  false,  true,   true,   false,  true,   true},    //TWO
  {true,  false,  true,   true,   true,   true,   false},   //THREE
  {true,  true,   false,  true,   true,   false,  false},   //FOUR
  {true,  true,   true,   false,  true,   true,   false},   //FIVE
  {true,  true,   true,   false,  true,   true,   true},    //SIX
  {false, false,  true,   true,   true,   false,  false},   //SEVEN
  {true,  true,   true,   true,   true,   true,   true},    //EIGHT
  {true,  true,   true,   true,   true,   true,   false},   //NINE
  {true,  true,   true,   true,   false,  false,  false},   //Degree
  {false, true,   true,   false,  false,  true,   true},    //C
  {true,  true,   true,   true,   false,  false,  false},   //Up O (%)
  {true,  false,  false,  false,  true,   true,   true}     //Down O (%)
};

/************************ SKETCH **************************/

void setup() {
  Serial.begin(9600);
  Serial.println(F("Neopixel Clock Started!!!"));

  clock.begin();
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(Leds, NUM_LEDS);
  FastLED.setBrightness(bright);
  IrReceiver.begin(IRpin, ENABLE_LED_FEEDBACK);
  pinMode(Piezo, OUTPUT);

  // Clear EOSC bit to ensure that the clock runs on battery power
  Wire.beginTransmission(0x68); // address DS3231
  Wire.write(0x0E);             // select register
  Wire.write(0b00011100);       // write register bitmap, bit 7 is /EOSC
  Wire.endTransmission();

  /************************** TIME SETTINGS **************************/

  // Uncomment to set date and time. **After uploading COMMENT it back and uplode the code again.
  //clock.setDateTime(__DATE__, __TIME__);      // Set sketch compiling time
  //clock.setDateTime(2021, 9, 12, 15, 21, 0);  // Manual (Year, Month, Day, Hour, Minute, Second)

  /************************ ALARM SETTINGS ************************/

  //To set Alarm change the status TRUE then decleare the Alarm function.
  clock.armAlarm1(true);
  clock.armAlarm2(false);
  clock.clearAlarm1();
  clock.clearAlarm2();
  

  // Check alarm settings

  // Set Alarm - Every second.
  // DS3231_EVERY_SECOND is available only on Alarm1.
  // setAlarm1(Date or Day, Hour, Minute, Second, Mode, Armed = true)
  // clock.setAlarm1(0, 0, 0, 0, DS3231_EVERY_SECOND);

  // Set Alarm - Every full minute.
  // DS3231_EVERY_MINUTE is available only on Alarm2.
  // setAlarm2(Date or Day, Hour, Minute, Second, Mode, Armed = true)
  // clock.setAlarm2(0, 0, 0, 0, DS3231_EVERY_MINUTE);
  
  // Set Alarm1 - Every 20s in each minute
  // setAlarm1(Date or Day, Hour, Minute, Second, Mode, Armed = true)
  //clock.setAlarm1(0, 0, 0, 20, DS3231_MATCH_S);

  // Set Alarm2 - Every 01m in each hour
  // setAlarm2(Date or Day, Hour, Minute, Mode, Armed = true)
  //clock.setAlarm2(0, 0, 1,     DS3231_MATCH_M);

  // Set Alarm - Every 01m:25s in each hour
  // setAlarm1(Date or Day, Hour, Minute, Second, Mode, Armed = true)
  // clock.setAlarm1(0, 0, 1, 25, DS3231_MATCH_M_S);

  // Set Alarm - Every 06h:0m:0s in each day
  // setAlarm1(Date or Day, Hour, Minute, Second, Mode, Armed = true)
  clock.setAlarm1(0, 6, 0, 0, DS3231_MATCH_H_M_S);

  // Set Alarm - 07h:00m:00s in 25th day in month
  // setAlarm1(Date or Day, Hour, Minute, Second, Mode, Armed = true)
  // clock.setAlarm1(25, 7, 0, 0, DS3231_MATCH_DT_H_M_S);

  // Set Alarm - 10h:45m:30s in every Friday (1 - Mon, 7 - Sun)
  // setAlarm1(Date or Day, Hour, Minute, Second, Mode, Armed = true)
  // clock.setAlarm1(5, 10, 40, 30, DS3231_MATCH_DY_H_M_S);

}

void loop() {
  EVERY_N_MILLISECONDS(250) { //Updates every 0.25sec
    FastRemote();    
    setDisplay();
    if (clock.isAlarm1()) // Rings when Alarm Triggered
      ring('A');
  }
}
 // Pizo Buzzer events Tones
void ring(char Tone) {
  if (Tone == 'T') {      // Beep
    digitalWrite(Piezo, HIGH);
    delay(2);
    digitalWrite(Piezo, LOW);
  }
  else if (Tone == 'H') { // Every new Hour
    digitalWrite(Piezo, HIGH);
    delay(250);
    digitalWrite(Piezo, LOW);
  }
  else if (Tone == 'A') { // Alarm Ringtone
    digitalWrite(Piezo, HIGH);
    delay(500);
    digitalWrite(Piezo, LOW);
    delay(250);
    digitalWrite(Piezo, HIGH);
    delay(500);
    digitalWrite(Piezo, LOW);
  }
}

/* This function checks led values to turn on (according to seg[][])
 * adds led values to ONleds array and these led are annimated 
 * with hue value colour arrangements
 */
void displayDigit(int Info) {
  ONleds = 0;
  for (int index = 0; index < 4; index++) {
    for (int i = 0; i < 7; i++) {
      if (seg[Info % 10][i]) {
        int fstLed = i * LEDS_PER_SEG + digits[index];
        for (int j = 0; j < LEDS_PER_SEG; j++) {
          OLEDS[ONleds] = fstLed + j;
          ONleds++;
        }
      }
    }
    Info /= 10;
    if (Info == 0) {
      break;
    }
  }
  // SETTING LED COLORS
  for (int i = 0; i < ONleds; i++) {
    Leds[OLEDS[i]] = CHSV(hue + (i * 5), 255, 255);
  }
  hue+=2;
}

// Set the display according to the Mode selected 
void setViewto(char x) { // X : Mode key { T : Time | D : Date }
  if (x == 'T') {        

    // 12 Hr 
    int HOUR = dt.hour;
    if (HOUR == 12 || HOUR == 0) {
      displayDigit((12 * 100) + dt.minute);
    } else {
      displayDigit(((HOUR % 12) * 100) + dt.minute);
    }

    // 24 Hr
    //displayDigit((dt.hour) * 100 + dt.minute);

    if (dt.second % 2 == 0) { // Seconds leds 
      Leds[7 * LEDS_PER_SEG * 2 + 0] = CHSV(hue + (7 * 2), 255, 255);
      Leds[7 * LEDS_PER_SEG * 2 + 1] = CHSV(hue + (7 * 2), 255, 255);
    }

    if (dt.minute == 0 && dt.second == 0) { // Every new Hour Buzzer Rings
      ring('H');    // H : Hour Tone
      delay(500);
    }
  } else if (x == 'D') {   // display Date
    displayDigit((dt.month * 100) + dt.day); // FORMAT MM-DD
    //displayDigit((dt.day * 100) + dt.month); // FORMAT DD-MM
  }
}

// Delay function to switch back to Default mode
void setBack(char c, int delayTime) {
  if (millis() > (current + delayTime)) {
    if (c == 'M') {
      mode = 0;
    }
  }
}

// Set Display to Show Time, date
void setDisplay() {
  dt = clock.getDateTime();
  FastLED.clear();
  if (mode == 0) {      // MODE 0 : Display Time
    setViewto('T');
  } 
  else if (mode == 1){  // MODE 1 : Display Date
    setViewto('D');
    setBack('M', 2000); // After 2sec Display switch back to Time mode
  }
  FastLED.show();
}

//************ COMMUNICATION PROTOCOL ************//
void FastRemote() {
  if (IrReceiver.decode()) {
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);
    IrReceiver.resume();

    /****** FOR LONG KEY PRESS******/
    if (IrReceiver.decodedIRData.decodedRawData == 0) {
      IrReceiver.decodedIRData.decodedRawData = lastCode;
    } else {
      lastCode = IrReceiver.decodedIRData.decodedRawData;
    }

    switch (IrReceiver.decodedIRData.decodedRawData) {
      //****** BRIGHTNESS CHANGE ******//
      case BrightUP:
        if (bright < 250) {
          bright += 5;
          ring('T');
          //Serial.println(bright);
          FastLED.setBrightness(bright);
        }
        break;

      case BrightDN:
        if (bright > 5) {
          bright -= 5;
          ring('T');
          //Serial.println(bright);
          FastLED.setBrightness(bright);
        }
        break;

      /****** MENU CHANGE ******/
      case Mode:
        ring('T');
        if (mode == 0) {
          mode = 1;
          current = millis();
          break;
        } else if (mode == 1) {
          mode = 0;
          FastLED.clear();
          setViewto('T');
          break;
        }

      default:
        //Serial.println(F("UnknownIR"));
        break;
    }
  }
}
