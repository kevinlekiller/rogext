/**
 * Copyright (C) 2018  kevinlekiller
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Wire.h>

// Change if you don't have a AMD CPU.
#define CPU_TYPE AMD

#if CPU_TYPE == AMD
  #define VDIV1 0.6475
  #define VDIV2 0.0025
#else
  #define VDIV1 1.28
  #define VDIV2 0.005
#endif

#define CDIV1 25.6
#define CDIV2 0.1

#define TVOLT    0x00
#define TCLOCK   0x01
#define TRATIO   0x02
#define TFAN     0x03
#define TTEMP    0x04
#define TQCODE   0x05
#define TOPEID   0x06

#define ROG_EXT 0x4a

#define ADDRESSES 11

typedef struct {
  int temp;
  int pwm;
} pwmTemp;

/*************************************************************************************************
 * Start of settings.
 *************************************************************************************************/

// Print ROG_EXT sensor info to serial.
const char printRogInfo = 0;

// Delay between turning fan on to off. This is just to prevent the fan from turning off / on too frequently.
const int fanSwitchDelay = 3000;

// Milliseconds to sleep between calculations.
const int globalDelay = 250;

// Fan speed ramps us and down slowly, this controls how fast or slow that happens.
// Smaller number means the fan ramps up/down faster.
const int rampSpeed = 500;

// Digital pin to turn on / off the fan.
const int fanControlPin = 3;

// Digital pin for the fan PWM control, must be a pin with a timer,
// if you change this you must change OCR4C based on the pin you picked.
const int fanPwmPin = 8;

// PWM at which the fan hits 100% RPM (the fan will stay 100% rpm even if the PWM is higher than this).
const int maxPWM = 320;

// PWM at which the fan hits its minimum RPM (the fan will stay the same RPM even if the PWM is less than this).
const int minPWM = 48;

// Difference in temp between the max temp and min temp, 65-47 here for example.
// In this example, 47 is slightly higher than the temp the CPU idles at, this is to prevent
// the fans from starting. 65 is slightly higher than the max temp the CPU reaches under 100% load.
const int tempDiff = 18;
pwmTemp pwmTempPairs[4] = {
//Temp, PWM
  {47, minPWM}, // Min temp for the fan to start spinning, how much PWM to send it
  {53, 139}, // Values are interpolated based on this and the next set
  {59, 230},
  {65, maxPWM} // Temp for which to send the max PWM to the fan
};

/*************************************************************************************************
 * End of settings.
 *************************************************************************************************/

int fanCurve[tempDiff + 1];

typedef struct {
    int addr;
    char bytes;
    char type;
    char * name;
} addrStruct;

addrStruct addresses[ADDRESSES] = {
    {0x00, 1, TOPEID, "OPEID"},
//    {0x01, 1
//    {0x07, 1
    {0x10, 1, TQCODE, "QCODE"},
//    {0x12, 1
    {0x20, 1, TRATIO, "CPU Ratio"},
//    {0x22, 1
    {0x24, 1, TRATIO, "Cache Ratio"},
    {0x28, 2, TCLOCK, "BCLK"},
//    {0x2a, 2 ; PCIEBCLK?
//    {0x2c, 2
//    {0x2e, 2
    {0x30, 2, TVOLT,  "V1"},
//    {0x32, 2
//    {0x34, 2
//    {0x36, 2
    {0x38, 2, TVOLT,  "V2"},
//    {0x3a, 2
//    {0x3c, 2 ; 1.8v
//    {0x3e, 2
    {0x40, 2, TVOLT,  "VCORE"},
//    {0x42, 2
//    {0x44, 2
//    {0x46, 2
    {0x48, 2, TVOLT,  "VDRAM"},
//    {0x4c, 2
    {0x50, 1, TTEMP,  "CPU Temp"},
    {0x60, 2, TFAN,   "CPU Fanspeed"},
};

byte buf1, buf2, lastTemp;
int lastPWM = 0;
char fanStatus = 0;

void getI2cBytes(int, char);
void printVoltage();
void printFanSpeed();
void printClock();
void printTemp();
void printRatio();
void printQcode();
void processCurve();
void setFanPWM(int);

void setup()
{
  Serial.begin(9600);
  TCCR4A = TCCR4B = TCNT4 = 0;
  ICR4   = (F_CPU / 25000) / 2;
  OCR4C  = ICR4 / 2;
  TCCR4A = _BV(COM4C1) | _BV(WGM41);
  TCCR4B = _BV(WGM43)  | _BV(CS40);
  Wire.begin();
  pinMode(fanPwmPin, OUTPUT);
  pinMode(fanControlPin, OUTPUT);
  processCurve();
}

void loop()
{
  delay(globalDelay);
  if (printRogInfo) {
    for (int i = 0; i < ADDRESSES; i++) {
      Serial.print("Addr: ");
      Serial.print(addresses[i].addr, HEX);
      Serial.print(" ; Byte1: ");
      getI2cBytes(addresses[i].addr, addresses[i].bytes);
      Serial.print(buf1);
      if (addresses[i].bytes == 2) {
        Serial.print(" ; Byte2: ");
        Serial.print(buf2);
      }
      Serial.print(" ; ");
      Serial.print(addresses[i].name);
      Serial.print(" ");
      switch (addresses[i].type) {
        case TVOLT:
          printVoltage();
          break;
        case TFAN:
          printFanSpeed();
          break;
        case TTEMP:
          printTemp();
          break;
        case TCLOCK:
          printClock();
          break;
        case TRATIO:
          printRatio();
          break;
        case TQCODE:
          printQcode();
          break;
        default:
          break;
      }
      Serial.print("\n");
    }
  }
  getI2cBytes(addresses[9].addr, addresses[9].bytes);
  buf1 = buf1 > pwmTempPairs[3].temp ? pwmTempPairs[3].temp : buf1;
  
  if (buf1 < pwmTempPairs[0].temp) {
    if (fanStatus) {
      fanStatus = 0;
      digitalWrite(fanControlPin, LOW);
      Serial.println("Turning fan off");
      delay(fanSwitchDelay);
    }
    return;
  }

  if (lastTemp == buf1) {
    return;
  }

  lastTemp = buf1;

  int wPWM = fanCurve[int(floor(buf1) - pwmTempPairs[0].temp)];
  if (wPWM == lastPWM) {
    return;
  }
  Serial.print("Turning fan on, wanted PWM is: ");
  Serial.print(wPWM);
  Serial.print(", CPU temp is: ");
  Serial.print(buf1);
  Serial.println(" C");
  if (!fanStatus) {
    digitalWrite(fanControlPin, HIGH);
    fanStatus = 1;
  }
  setFanPWM(wPWM);
}

void getI2cBytes(int addr, char bytes)
{
  buf1 = buf2 = 0;
  Wire.beginTransmission(ROG_EXT);
  if (Wire.write(addr) && Wire.endTransmission() == 0 && Wire.requestFrom(ROG_EXT, bytes) == bytes && Wire.available() == bytes) {
    buf1 = Wire.read();
    if (bytes == 2) {
      buf2 = Wire.read();
    }
  }
}

void printVoltage()
{
  Serial.print(((buf1 * VDIV1) + (buf2 * VDIV2)), 4);
  Serial.print("V");
}

void printFanSpeed()
{
  Serial.print((buf1 << 8 | buf2));
  Serial.print("RPM");
}

void printTemp()
{
  Serial.print(buf1);
  Serial.print("C");
}

void printClock()
{
  Serial.print(((buf1 * CDIV1) + (buf2 * CDIV2)), 2);
  Serial.print("MHz");
}

void printRatio()
{
  Serial.print(buf1);
  Serial.print("x");
}

void printQcode()
{
  Serial.print("#");
  Serial.print(buf1);
}

// Calculates what pwm to send to the fan at what temperature.
// The pwm of the fan is calculated for every degree celcius and outputed to the serial interface based on above values.
void processCurve()
{
  int minTemp = pwmTempPairs[0].temp;
  float pwmDiff = (float)(maxPWM - minPWM) / tempDiff;
  for (int i = 0; i < 3; i++) {
    int tempLow = pwmTempPairs[i].temp;
    int tempHigh = pwmTempPairs[i+1].temp - (i == 2 ? 0 : 1);
    int pwmLow = pwmTempPairs[i].pwm;
    int pwmHigh = pwmTempPairs[i+1].pwm;
    float pwmOne = pwmLow;
    for (int j = tempLow - minTemp; j <= tempHigh - minTemp; j++) {
      float pwmTwo = pwmOne;
      if (pwmTwo <= pwmLow) {
        fanCurve[j] = pwmLow;
      } else if (pwmTwo >= pwmHigh) {
        fanCurve[j] = pwmHigh;
      } else {
        fanCurve[j] = round(pwmTwo);
      }
      pwmOne = pwmDiff + pwmOne;
    }
  }
  for (int i = 0; i <= tempDiff; i++) {
    Serial.print(i);
    Serial.print(" Temp: ");
    Serial.print(i+minTemp);
    Serial.print(" Pwm: ");
    Serial.println(fanCurve[i]);
    delay(10);
  }
}

// Gradually ramps up / down fan speed.
void setFanPWM(int value)
{
  if (lastPWM) {
    int curPWM = lastPWM;
    int diff = lastPWM > value ? lastPWM - value : value - lastPWM;
    char positive = value > lastPWM;
    int iters = diff / 10;
    diff /= iters;
    for (int i = 0; i <= iters; i++) {
      curPWM = positive ? curPWM + diff : curPWM - diff;
      OCR4C = curPWM;
      delay(rampSpeed);
    }
  }
  lastPWM = value;
  OCR4C = value;
}
