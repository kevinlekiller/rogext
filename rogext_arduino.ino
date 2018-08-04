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

byte buf1, buf2;

void getI2cBytes(int, char);
void printVoltage();
void printFanSpeed();
void printClock();
void printTemp();
void printRatio();
void printQcode();

void setup()
{
  Serial.begin(9600);
  Wire.begin();
}

void loop()
{

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
  delay(1000);
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
