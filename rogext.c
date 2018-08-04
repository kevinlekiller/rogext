
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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define CPUTYPE AMD

#if CPUTYPE == AMD
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

//i2cdump 1 0x4a
/*
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f    0123456789abcdef
00: 00 0f 00 00 00 00 00 04 00 00 00 00 00 00 00 00    .?.....?........
10: 00 00 06 00 00 00 00 00 00 00 00 00 00 00 00 00    ..?.............
20: 25 25 3f 20 00 00 00 00 03 e8 03 e8 0b b8 01 90    %%? ....????????
30: 01 90 00 c8 01 68 00 96 01 68 01 68 02 d0 01 2c    ??.??h.??h?h???,
40: 01 15 01 14 01 54 00 96 02 2a 00 f0 01 e0 00 64    ?????T.??*.???.d
50: 22 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    "...............
60: 00 fc 00 00 00 00 00 00 00 00 00 00 00 00 00 00    .?..............
70: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
90: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
a0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
b0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
c0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
d0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
e0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
f0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
*/
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

void printVoltage();
void printFanSpeed();
void printClock();
void printTemp();
void printRatio();
void printQcode();

__s32 buf1, buf2;

int main() {
    int fd = open("/dev/i2c-1", O_RDWR);
    if (!fd) {
        fprintf(stderr, "Can not open i2c-1: %d\n", fd);
        close(fd);
        return 1;
    }
    if (ioctl(fd, I2C_SLAVE, ROG_EXT)) {
        fprintf(stderr, "Can not access i2c device\n");
        close(fd);
        return 1;
    }
    char buf3[2];
    for (int i = 0; i < ADDRESSES; i++) {
        printf("Addr: %02x ; Byte1: ", addresses[i].addr);
        if (addresses[i].bytes == 1) {
            buf1 = i2c_smbus_read_byte_data(fd, addresses[i].addr);
            printf("%d",  buf1);
        } else {
            i2c_smbus_read_i2c_block_data(fd, addresses[i].addr, 2, buf3);
            buf1 = (__s32) buf3[0];
            buf2 = (__s32) buf3[1];
            printf("%d ; Byte2 : %d", buf1, buf2);
        }
        printf(" ; %s ", addresses[i].name);
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
        printf("\n");
    }
    close(fd);
}

void printVoltage() {
    printf("%.4fV", ((buf1 * VDIV1) + (buf2 * VDIV2)));
}

void printFanSpeed() {
    printf("%dRPM", (buf1 << 8 | buf2));
}

void printTemp() {
    printf("%dC", buf1);
}

void printClock() {
    printf("%.2fMHz", ((buf1 * CDIV1) + (buf2 * CDIV2)));
}

void printRatio() {
    printf("%dx", buf1);
}

void printQcode() {
    printf("#%d", buf1);
}
