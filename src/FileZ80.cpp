///////////////////////////////////////////////////////////////////////////////
//
// ZX-ESPectrum - ZX Spectrum emulator for ESP32
//
// Copyright (c) 2020, 2021 David Crespo [dcrespo3d]
// https://github.com/dcrespo3d/ZX-ESPectrum-Wiimote
//
// Based on previous work by Ramón Martinez, Jorge Fuertes and many others
// https://github.com/rampa069/ZX-ESPectrum
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include "hardconfig.h"
#include "FileZ80.h"
#include <FS.h>
#include "FileUtils.h"
#include "PS2Kbd.h"
#include "CPU.h"
#include "Mem.h"
#include "ESPectrum.h"
#include "messages.h"
#include "osd.h"
#include <FS.h>
#include "Wiimote2Keys.h"
#include "Config.h"
#include "FileUtils.h"

///////////////////////////////////////////////////////////////////////////////

#ifdef CPU_LINKEFONG
#include "Z80_LKF/z80emu.h"
extern Z80_STATE _zxCpu;
#endif

///////////////////////////////////////////////////////////////////////////////

#ifdef CPU_JLSANCHEZ
#include "Z80_JLS/z80.h"
#endif

///////////////////////////////////////////////////////////////////////////////

#ifdef USE_INT_FLASH
// using internal storage (spi flash)
#include <SPIFFS.h>
// set The Filesystem to SPIFFS
#define THE_FS SPIFFS
#endif

///////////////////////////////////////////////////////////////////////////////

#ifdef USE_SD_CARD
// using external storage (SD card)
#include <SD.h>
// set The Filesystem to SD
#define THE_FS SD
#endif

///////////////////////////////////////////////////////////////////////////////

static uint16_t mkword(uint8_t lobyte, uint8_t hibyte) {
    return lobyte | (hibyte << 8);
}

bool FileZ80::load(String sna_fn)
{
    KB_INT_STOP;

    if (sna_fn != DISK_PSNA_FILE)
        loadKeytableForGame(sna_fn.c_str());

    Serial.println("FileZ80::load");
    File f = FileUtils::safeOpenFileRead(sna_fn);
    uint32_t file_size = f.size();

    uint32_t dataOffset = 0;

    // initially assuming version 1; this assumption may change
    uint8_t version = 1;

    // stack space for header, should be enough for
    // version 1 (30 bytes)
    // version 2 (55 bytes) (30 + 2 + 23)
    // version 3 (87 bytes) (30 + 2 + 55) or (86 bytes) (30 + 2 + 54)
    uint8_t header[87];

    // read first 30 bytes
    for (uint8_t i = 0; i < 30; i++) {
        header[i] = f.read();
        dataOffset ++;
    }

    // additional vars
    uint8_t b12, b29;

    // begin loading registers
#ifdef CPU_LINKEFONG
    _zxCpu.registers.byte[Z80_A]  =        header[0];
    _zxCpu.registers.byte[Z80_F]  =        header[1];
    _zxCpu.registers.word[Z80_BC] = mkword(header[2], header[3]);
    _zxCpu.registers.word[Z80_HL] = mkword(header[4], header[5]);
    _zxCpu.pc                     = mkword(header[6], header[7]);
    _zxCpu.registers.word[Z80_SP] = mkword(header[8], header[9]);
    _zxCpu.i                      =        header[10];
    _zxCpu.r                      =        header[11];
    b12                           =        header[12];
    _zxCpu.registers.word[Z80_DE] = mkword(header[13], header[14]);
    _zxCpu.alternates    [Z80_BC] = mkword(header[15], header[16]);
    _zxCpu.alternates    [Z80_DE] = mkword(header[17], header[18]);
    _zxCpu.alternates    [Z80_HL] = mkword(header[19], header[20]);
    _zxCpu.alternates    [Z80_AF] = mkword(header[22], header[21]); // watch out for order!!!
    _zxCpu.registers.word[Z80_IY] = mkword(header[23], header[24]);
    _zxCpu.registers.word[Z80_IX] = mkword(header[25], header[26]);
    _zxCpu.iff1                   =        header[27];
    _zxCpu.iff2                   =        header[28];
    b29                           =        header[29];
    _zxCpu.im                     = (b29 & 0x03);

    uint16_t RegPC = _zxCpu.pc;
#endif // CPU_LINKEFONG

#ifdef CPU_JLSANCHEZ
    Z80::setRegA  (       header[0]);
    Z80::setFlags (       header[1]);
    Z80::setRegBC (mkword(header[2], header[3]));
    Z80::setRegHL (mkword(header[4], header[5]));
    Z80::setRegPC (mkword(header[6], header[7]));
    Z80::setRegSP (mkword(header[8], header[9]));
    Z80::setRegI  (       header[10]);
    Z80::setRegR  (       header[11]);
    b12 =                 header[12];
    Z80::setRegDE (mkword(header[13], header[14]));
    Z80::setRegBCx(mkword(header[15], header[16]));
    Z80::setRegDEx(mkword(header[17], header[18]));
    Z80::setRegHLx(mkword(header[19], header[20]));
    Z80::setRegAFx(mkword(header[22], header[21])); // watch out for order!!!
    Z80::setRegIY (mkword(header[23], header[24]));
    Z80::setRegIX (mkword(header[25], header[26]));
    Z80::setIFF1  (       header[27] ? true : false);
    Z80::setIFF2  (       header[28] ? true : false);
    b29 =                 header[29];
    Z80::setIM((Z80::IntMode)(b29 & 0x03));

    uint16_t RegPC = Z80::getRegPC();
#endif // CPU_JLSANCHEZ


    ESPectrum::borderColor = (b12 >> 1) & 0x07;

    bool dataCompressed = (b12 & 0x20) ? true : false;
    String fileArch = "48K";
    uint8_t memPagingReg = 0;

// #define LOG_Z80_DETAILS

    if (RegPC != 0)
    {
        // version 1, the simplest, 48K only.
        uint32_t memRawLength = file_size - dataOffset;
#ifdef LOG_Z80_DETAILS
        Serial.printf("Z80 format version: %d\n", version);
        Serial.printf("machine type: %s\n", fileArch);
        Serial.printf("data offset: %d\n", dataOffset);
        Serial.printf("data compressed: %s\n", dataCompressed ? "true" : "false");
        Serial.printf("file length: %d\n", file_size);
        Serial.printf("data length: %d\n", memRawLength);
        Serial.printf("b12: %d\n", b12);
        Serial.printf("pc: %d\n", RegPC);
        Serial.printf("border: %d\n", ESPectrum::borderColor);
#endif

        if (dataCompressed)
        {
            // assuming stupid 00 ED ED 00 terminator present, should check for it instead of assuming
            uint16_t dataLen = (uint16_t)(memRawLength - 4);

            // load compressed data into memory
            loadCompressedMemData(f, dataLen, 0x4000, 0xC000);
        }
        else
        {
            uint16_t dataLen = (memRawLength < 0xC000) ? memRawLength : 0xC000;

            // load uncompressed data into memory
            for (int i = 0; i < dataLen; i++)
                Mem::writebyte(0x4000 + i, f.read());
        }

        // latches for 48K
        Mem::romLatch = 0;
        Mem::romInUse = 0;
        Mem::bankLatch = 0;
        Mem::pagingLock = 1;
        Mem::videoLatch = 0;
    }
    else
    {
        // read 2 more bytes
        for (uint8_t i = 30; i < 32; i++) {
            header[i] = f.read();
            dataOffset ++;
        }

        // additional header block length
        uint16_t ahblen = mkword(header[30], header[31]);
        if (ahblen == 23)
            version = 2;
        else if (ahblen == 54 || ahblen == 55)
            version = 3;
        else {
            Serial.printf("Z80.load: unknown version, ahblen = %d\n", ahblen);
            return false;
        }

        // read additional header block
        for (uint8_t i = 32; i < 32 + ahblen; i++) {
            header[i] = f.read();
            dataOffset ++;
        }

        // program counter
        RegPC = mkword(header[32], header[33]);
#ifdef CPU_LINKEFONG
        _zxCpu.pc = RegPC;
#endif // CPU_LINKEFONG

#ifdef CPU_JLSANCHEZ
        Z80::setRegPC(RegPC);
#endif // CPU_JLSANCHEZ

        // hardware mode
        uint8_t b34 = header[34];
        // defaulting to 128K
        fileArch = "128K";

        if (version == 2) {
            if (b34 == 0) fileArch = "48K";
            if (b34 == 1) fileArch = "48K"; // + if1
            if (b34 == 2) fileArch = "SAMRAM";
        }
        else if (version == 3) {
            if (b34 == 0) fileArch = "48K";
            if (b34 == 1) fileArch = "48K"; // + if1
            if (b34 == 2) fileArch = "SAMRAM";
            if (b34 == 3) fileArch = "48K"; // + mgt
        }

#ifdef LOG_Z80_DETAILS
        uint32_t memRawLength = file_size - dataOffset;
        Serial.printf("Z80 format version: %d\n", version);
        Serial.printf("machine type: %s\n", fileArch);
        Serial.printf("data offset: %d\n", dataOffset);
        Serial.printf("file length: %d\n", file_size);
        Serial.printf("b12: %d\n", b12);
        Serial.printf("b34: %d\n", b34);
        Serial.printf("pc: %d\n", RegPC);
        Serial.printf("border: %d\n", ESPectrum::borderColor);
#endif

        if (fileArch == "48K") {
            Mem::romLatch = 0;
            Mem::romInUse = 0;
            Mem::bankLatch = 0;
            Mem::pagingLock = 1;
            Mem::videoLatch = 0;

            uint16_t pageStart[12] = {0, 0, 0, 0, 0x8000, 0xC000, 0, 0, 0x4000, 0, 0};

            uint32_t dataLen = file_size;
            while (dataOffset < dataLen) {
                uint8_t hdr0 = f.read(); dataOffset ++;
                uint8_t hdr1 = f.read(); dataOffset ++;
                uint8_t hdr2 = f.read(); dataOffset ++;
                uint16_t compDataLen = mkword(hdr0, hdr1);
#ifdef LOG_Z80_DETAILS
                Serial.printf("compressed data length: %d\n", compDataLen);
                Serial.printf("page number: %d\n", hdr2);
#endif
                uint16_t memoff = pageStart[hdr2];
#ifdef LOG_Z80_DETAILS
                Serial.printf("page start: 0x%X\n", memoff);
#endif
                loadCompressedMemData(f, compDataLen, memoff, 0x4000);
                dataOffset += compDataLen;
            }

            // great success!!!
        }
        else if (fileArch == "128K") {
            Mem::romInUse = 1;

            // paging register
            uint8_t b35 = header[35];
            Mem::pagingLock = bitRead(b35, 5);
            Mem::romLatch = bitRead(b35, 4);
            Mem::videoLatch = bitRead(b35, 3);
            Mem::bankLatch = b35 & 0x07;

            uint8_t* pages[12] = {
                Mem::rom0, Mem::rom2, Mem::rom1,
                Mem::ram0, Mem::ram1, Mem::ram2, Mem::ram3,
                Mem::ram4, Mem::ram5, Mem::ram6, Mem::ram7,
                Mem::rom3 };

            const char* pagenames[12] = { "rom0", "IDP", "rom1",
                "ram0", "ram1", "ram2", "ram3", "ram4", "ram5", "ram6", "ram7", "MFR" };

#ifdef LOG_Z80_DETAILS

#endif
            uint32_t dataLen = file_size;
            while (dataOffset < dataLen) {
                uint8_t hdr0 = f.read(); dataOffset ++;
                uint8_t hdr1 = f.read(); dataOffset ++;
                uint8_t hdr2 = f.read(); dataOffset ++;
                uint16_t compDataLen = mkword(hdr0, hdr1);
#ifdef LOG_Z80_DETAILS
                Serial.printf("compressed data length: %d\n", compDataLen);
                Serial.printf("page: %s\n", pagenames[hdr2]);
#endif
                uint8_t* memPage = pages[hdr2];

                loadCompressedMemPage(f, compDataLen, memPage, 0x4000);
                dataOffset += compDataLen;
            }

            // great success!!!
        }
    }

    // just architecturey things
    if (Config::getArch() == "128K")
    {
        if (fileArch == "48K")
        {
#ifdef SNAPSHOT_LOAD_FORCE_ARCH
            Config::requestMachine("48K", "SINCLAIR", true);
            Mem::romInUse = 0;
#else
            Mem::romInUse = 1;
#endif
        }
    }
    else if (Config::getArch() == "48K")
    {
        if (fileArch == "128K")
        {
            Config::requestMachine("128K", "SINCLAIR", true);
            Mem::romInUse = 1;
        }
    }

    delay(100);

    KB_INT_START;

    return true;
}

void FileZ80::loadCompressedMemData(File f, uint16_t dataLen, uint16_t memoff, uint16_t memlen)
{
    uint16_t dataOff = 0;
    uint8_t ed_cnt = 0;
    uint8_t repcnt = 0;
    uint8_t repval = 0;
    uint16_t memidx = 0;

    while(dataOff < dataLen && memidx < memlen) {
        uint8_t databyte = f.read();
        if (ed_cnt == 0) {
            if (databyte != 0xED)
                Mem::writebyte(memoff + memidx++, databyte);
            else
                ed_cnt++;
        }
        else if (ed_cnt == 1) {
            if (databyte != 0xED) {
                Mem::writebyte(memoff + memidx++, 0xED);
                Mem::writebyte(memoff + memidx++, databyte);
                ed_cnt = 0;
            }
            else
                ed_cnt++;
        }
        else if (ed_cnt == 2) {
            repcnt = databyte;
            ed_cnt++;
        }
        else if (ed_cnt == 3) {
            repval = databyte;
            for (uint16_t i = 0; i < repcnt; i++)
                Mem::writebyte(memoff + memidx++, repval);
            ed_cnt = 0;
        }
    }
    #ifdef LOG_Z80_DETAILS
        Serial.printf("last byte: %d\n", (memoff+memidx-1));
    #endif
}

void FileZ80::loadCompressedMemPage(File f, uint16_t dataLen, uint8_t* memPage, uint16_t memlen)
{
    uint16_t dataOff = 0;
    uint8_t ed_cnt = 0;
    uint8_t repcnt = 0;
    uint8_t repval = 0;
    uint16_t memidx = 0;

    while(dataOff < dataLen && memidx < memlen) {
        uint8_t databyte = f.read();
        if (ed_cnt == 0) {
            if (databyte != 0xED)
                memPage[memidx++] = databyte;
            else
                ed_cnt++;
        }
        else if (ed_cnt == 1) {
            if (databyte != 0xED) {
                memPage[memidx++] = 0xED;
                memPage[memidx++] = databyte;
                ed_cnt = 0;
            }
            else
                ed_cnt++;
        }
        else if (ed_cnt == 2) {
            repcnt = databyte;
            ed_cnt++;
        }
        else if (ed_cnt == 3) {
            repval = databyte;
            for (uint16_t i = 0; i < repcnt; i++)
                memPage[memidx++] = repval;
            ed_cnt = 0;
        }
    }
    #ifdef LOG_Z80_DETAILS
        Serial.printf("last byte: %d\n", (memoff+memidx-1));
    #endif
}
