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
#include "FileSNA.h"

///////////////////////////////////////////////////////////////////////////////

#ifdef CPU_LINKEFONG
#include "Z80_LKF/z80emu.h"
extern Z80_STATE _zxCpu;

#define Z80_SET_AF(v) _zxCpu.registers.word[Z80_AF] = (v)
#define Z80_SET_BC(v) _zxCpu.registers.word[Z80_BC] = (v)
#define Z80_SET_DE(v) _zxCpu.registers.word[Z80_DE] = (v)
#define Z80_SET_HL(v) _zxCpu.registers.word[Z80_HL] = (v)

#define Z80_SET_AFx(v) _zxCpu.alternates[Z80_AF] = (v)
#define Z80_SET_BCx(v) _zxCpu.alternates[Z80_BC] = (v)
#define Z80_SET_DEx(v) _zxCpu.alternates[Z80_DE] = (v)
#define Z80_SET_HLx(v) _zxCpu.alternates[Z80_HL] = (v)

#define Z80_SET_IY(v) _zxCpu.registers.word[Z80_IY] = (v)
#define Z80_SET_IX(v) _zxCpu.registers.word[Z80_IX] = (v)

#define Z80_SET_SP(v) _zxCpu.registers.word[Z80_SP] = (v)
#define Z80_SET_PC(v) _zxCpu.pc = (v)

#define Z80_SET_I(v) _zxCpu.i = (v)
#define Z80_SET_R(v) _zxCpu.r = (v)
#define Z80_SET_IM(v) _zxCpu.im = (v)
#define Z80_SET_IFF1(v) _zxCpu.iff1 = (v)
#define Z80_SET_IFF2(v) _zxCpu.iff2 = (v)

#define Z80_GET_AF() (_zxCpu.registers.word[Z80_AF])
#define Z80_GET_BC() (_zxCpu.registers.word[Z80_BC])
#define Z80_GET_DE() (_zxCpu.registers.word[Z80_DE])
#define Z80_GET_HL() (_zxCpu.registers.word[Z80_HL])

#define Z80_GET_AFx() (_zxCpu.alternates[Z80_AF])
#define Z80_GET_BCx() (_zxCpu.alternates[Z80_BC])
#define Z80_GET_DEx() (_zxCpu.alternates[Z80_DE])
#define Z80_GET_HLx() (_zxCpu.alternates[Z80_HL])

#define Z80_GET_IY() (_zxCpu.registers.word[Z80_IY])
#define Z80_GET_IX() (_zxCpu.registers.word[Z80_IX])

#define Z80_GET_SP() (_zxCpu.registers.word[Z80_SP])
#define Z80_GET_PC() (_zxCpu.pc)

#define Z80_GET_I() (_zxCpu.i)
#define Z80_GET_R() (_zxCpu.r)
#define Z80_GET_IM() (_zxCpu.im)
#define Z80_GET_IFF1() (_zxCpu.iff1)
#define Z80_GET_IFF2() (_zxCpu.iff2)

#endif // CPU_LINKEFONG

///////////////////////////////////////////////////////////////////////////////

#ifdef CPU_JLSANCHEZ
#include "Z80_JLS/z80.h"

#define Z80_GET_AF() Z80::getRegAF()
#define Z80_GET_BC() Z80::getRegBC()
#define Z80_GET_DE() Z80::getRegDE()
#define Z80_GET_HL() Z80::getRegHL()

#define Z80_GET_AFx() Z80::getRegAFx()
#define Z80_GET_BCx() Z80::getRegBCx()
#define Z80_GET_DEx() Z80::getRegDEx()
#define Z80_GET_HLx() Z80::getRegHLx()

#define Z80_GET_IY() Z80::getRegIY()
#define Z80_GET_IX() Z80::getRegIX()

#define Z80_GET_SP() Z80::getRegSP()
#define Z80_GET_PC() Z80::getRegPC()

#define Z80_GET_I() Z80::getRegI()
#define Z80_GET_R() Z80::getRegR()
#define Z80_GET_IM() Z80::getIM()
#define Z80_GET_IFF1() Z80::isIFF1()
#define Z80_GET_IFF2() Z80::isIFF2()

#define Z80_SET_AF(v) Z80::setRegAF(v)
#define Z80_SET_BC(v) Z80::setRegBC(v)
#define Z80_SET_DE(v) Z80::setRegDE(v)
#define Z80_SET_HL(v) Z80::setRegHL(v)

#define Z80_SET_AFx(v) Z80::setRegAFx(v)
#define Z80_SET_BCx(v) Z80::setRegBCx(v)
#define Z80_SET_DEx(v) Z80::setRegDEx(v)
#define Z80_SET_HLx(v) Z80::setRegHLx(v)

#define Z80_SET_IY(v) Z80::setRegIY(v)
#define Z80_SET_IX(v) Z80::setRegIX(v)

#define Z80_SET_SP(v) Z80::setRegSP(v)
#define Z80_SET_PC(v) Z80::setRegPC(v)

#define Z80_SET_I(v) Z80::setRegI(v)
#define Z80_SET_R(v) Z80::setRegR(v)
#define Z80_SET_IM(v) Z80::setIM((Z80::IntMode)(v))
#define Z80_SET_IFF1(v) Z80::setIFF1(v)
#define Z80_SET_IFF2(v) Z80::setIFF2(v)

#endif  // CPU_JLSANCHEZ

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

bool FileSNA::load(String sna_fn)
{
    File file;
    uint16_t retaddr;
    int sna_size;
    ESPectrum::reset();

    Serial.printf("%s SNA: %ub\n", MSG_FREE_HEAP_BEFORE, ESP.getFreeHeap());

    KB_INT_STOP;

    if (sna_fn != DISK_PSNA_FILE)
        loadKeytableForGame(sna_fn.c_str());

    file = FileUtils::safeOpenFileRead(sna_fn);
    sna_size = file.size();

    if (sna_size < SNA_48K_SIZE) {
        Serial.printf("FileSNA::load: bad SNA %s: size = %d < %d\n", sna_fn.c_str(), sna_size, SNA_48K_SIZE);
        KB_INT_START;
        return false;
    }

    String snapshotArch = "48K";

    Mem::bankLatch = 0;
    Mem::pagingLock = 1;
    Mem::videoLatch = 0;
    Mem::romLatch = 0;
    Mem::romInUse = 0;

    // Read in the registers
    Z80_SET_I(readByteFile(file));

    Z80_SET_HLx(readWordFileLE(file));
    Z80_SET_DEx(readWordFileLE(file));
    Z80_SET_BCx(readWordFileLE(file));
    Z80_SET_AFx(readWordFileLE(file));

    Z80_SET_HL(readWordFileLE(file));
    Z80_SET_DE(readWordFileLE(file));
    Z80_SET_BC(readWordFileLE(file));

    Z80_SET_IY(readWordFileLE(file));
    Z80_SET_IX(readWordFileLE(file));

    uint8_t inter = readByteFile(file);
    Z80_SET_IFF2((inter & 0x04) ? true : false);
    Z80_SET_IFF1(Z80_GET_IFF2());
    Z80_SET_R(readByteFile(file));

    Z80_SET_AF(readWordFileLE(file));
    Z80_SET_SP(readWordFileLE(file));

    Z80_SET_IM(readByteFile(file));

    ESPectrum::borderColor = readByteFile(file);

    // read 48K memory
    readBlockFile(file, Mem::ram5, 0x4000);
    readBlockFile(file, Mem::ram2, 0x4000);
    readBlockFile(file, Mem::ram0, 0x4000);

    if (sna_size == SNA_48K_SIZE)
    {
        snapshotArch = "48K";

        // in 48K mode, pop PC from stack
        uint16_t SP = Z80_GET_SP();
        Z80_SET_PC(Mem::readword(SP));
        Z80_SET_SP(SP + 2);
    }
    else
    {
        snapshotArch = "128K";

        // in 128K mode, recover stored PC
        Z80_SET_PC(readWordFileLE(file));

        // tmp_port contains page switching status, including current page number (latch)
        uint8_t tmp_port = readByteFile(file);
        uint8_t tmp_latch = tmp_port & 0x07;

        // copy what was read into page 0 to correct page
        memcpy(Mem::ram[tmp_latch], Mem::ram[0], 0x4000);

        uint8_t tr_dos = readByteFile(file);     // unused
        
        // read remaining pages
        for (int page = 0; page < 8; page++) {
            if (page != tmp_latch && page != 2 && page != 5) {
                readBlockFile(file, Mem::ram[page], 0x4000);
            }
        }

        // decode tmp_port
        Mem::videoLatch = bitRead(tmp_port, 3);
        Mem::romLatch = bitRead(tmp_port, 4);
        Mem::pagingLock = bitRead(tmp_port, 5);
        Mem::bankLatch = tmp_latch;
        Mem::romInUse = Mem::romLatch;

    }
    file.close();

    // just architecturey things
    if (Config::getArch() == "128K")
    {
        if (snapshotArch == "48K")
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
        if (snapshotArch == "128K")
        {
            Config::requestMachine("128K", "SINCLAIR", true);
            Mem::romInUse = 1;
        }
    }

    KB_INT_START;
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool FileSNA::isPersistAvailable()
{
    String filename = DISK_PSNA_FILE;
    return THE_FS.exists(filename.c_str());
}

///////////////////////////////////////////////////////////////////////////////

static bool IRAM_ATTR writeMemPage(uint8_t page, File file, bool blockMode)
{
    page = page & 0x07;
    uint8_t* buffer = Mem::ram[page];

    Serial.printf("writing page %d in [%s] mode\n", page, blockMode ? "block" : "byte");

    if (blockMode) {
        // writing blocks should be faster, but fails sometimes when flash is getting full.
        uint16_t bytesWritten = file.write(buffer, MEM_PG_SZ);
        if (bytesWritten != MEM_PG_SZ) {
            Serial.printf("error writing page %d: %d of %d bytes written\n", page, bytesWritten, MEM_PG_SZ);
            return false;
        }
    }
    else {
        for (int offset = 0; offset < MEM_PG_SZ; offset++) {
            uint8_t b = buffer[offset];
            if (1 != file.write(b)) {
                Serial.printf("error writing byte from page %d at offset %d\n", page, offset);
                return false;
            }
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool FileSNA::save(String sna_file) {
    // #define BENCHMARK_BOTH_SAVE_METHODS
    #ifndef BENCHMARK_BOTH_SAVE_METHODS
        // try to save using pages

        if (FileSNA::save(sna_file, true))
            return true;
        OSD::osdCenteredMsg(OSD_PSNA_SAVE_WARN, 2);
        // try to save byte-by-byte
        return FileSNA::save(sna_file, false);
    #else
        uint32_t ts_start, ts_end;
        bool ok;

        ts_start = millis();
        ok = FileSNA::save(sna_file, true);
        ts_end = millis();

        Serial.printf("save SNA [page]: ok = %d, millis = %d\n", ok, (ts_end - ts_start));

        ts_start = millis();
        ok = FileSNA::save(sna_file, false);
        ts_end = millis();

        Serial.printf("save SNA [byte]: ok = %d, millis = %d\n", ok, (ts_end - ts_start));

        return ok;
    #endif // BENCHMARK_BOTH_SAVE_METHODS
}

///////////////////////////////////////////////////////////////////////////////

bool FileSNA::save(String sna_file, bool blockMode) {
    KB_INT_STOP;

    // open file
    File file = THE_FS.open(sna_file, FILE_WRITE);
    if (!file) {
        Serial.printf("FileSNA::save: failed to open %s for writing\n", sna_file.c_str());
        KB_INT_START;
        return false;
    }

    // write registers: begin with I
    writeByteFile(Z80_GET_I(), file);

    writeWordFileLE(Z80_GET_HLx(), file);
    writeWordFileLE(Z80_GET_DEx(), file);
    writeWordFileLE(Z80_GET_BCx(), file);
    writeWordFileLE(Z80_GET_AFx(), file);

    writeWordFileLE(Z80_GET_HL(), file);
    writeWordFileLE(Z80_GET_DE(), file);
    writeWordFileLE(Z80_GET_BC(), file);

    writeWordFileLE(Z80_GET_IY(), file);
    writeWordFileLE(Z80_GET_IX(), file);

    uint8_t inter = Z80_GET_IFF2() ? 0x04 : 0;
    writeByteFile(inter, file);
    writeByteFile(Z80_GET_R(), file);

    writeWordFileLE(Z80_GET_AF(), file);

    uint16_t SP = Z80_GET_SP();
    if (Config::getArch() == "48K") {
        // decrement stack pointer it for pushing PC to stack, only on 48K
        SP -= 2;
        Mem::writeword(SP, Z80_GET_PC());
    }
    writeWordFileLE(SP, file);

    writeByteFile(Z80_GET_IM(), file);
    uint8_t bordercol = ESPectrum::borderColor;
    writeByteFile(bordercol, file);

    // write RAM pages in 48K address space (0x4000 - 0xFFFF)
    uint8_t pages[3] = {5, 2, 0};
    if (Config::getArch() == "128K")
        pages[2] = Mem::bankLatch;

    for (uint8_t ipage = 0; ipage < 3; ipage++) {
        uint8_t page = pages[ipage];
        if (!writeMemPage(page, file, blockMode)) {
            file.close();
            KB_INT_START;
            return false;
        }
    }

    if (Config::getArch() == "48K")
    {
        // nothing to do here
    }
    else if (Config::getArch() == "128K")
    {
        // write pc
        writeWordFileLE(Z80_GET_PC(), file);

        // write mem bank control port
        uint8_t tmp_port = Mem::bankLatch;
        bitWrite(tmp_port, 3, Mem::videoLatch);
        bitWrite(tmp_port, 4, Mem::romLatch);
        bitWrite(tmp_port, 5, Mem::pagingLock);
        writeByteFile(tmp_port, file);

        writeByteFile(0, file);     // TR-DOS not paged

        // write remaining ram pages
        for (int page = 0; page < 8; page++) {
            if (page != Mem::bankLatch && page != 2 && page != 5) {
                if (!writeMemPage(page, file, blockMode)) {
                    file.close();
                    KB_INT_START;
                    return false;
                }
            }
        }
    }

    file.close();
    KB_INT_START;
    return true;
}

///////////////////////////////////////////////////////////////////////////////

static uint8_t* quick_sna_buffer = NULL;
static uint32_t quick_sna_size   = 0;

bool FileSNA::isQuickAvailable()
{
    return quick_sna_buffer != NULL;
}

bool FileSNA::saveQuick()
{
    KB_INT_STOP;

    // deallocate buffer if it does not fit required size
    if (quick_sna_buffer != NULL)
    {
        if (quick_sna_size == SNA_48K_SIZE && Config::getArch() != "48K") {
            free(quick_sna_buffer);
            quick_sna_buffer = NULL;
            quick_sna_size = 0;
        }
        else if (quick_sna_size != SNA_48K_SIZE && Config::getArch() == "48K") {
            free(quick_sna_buffer);
            quick_sna_buffer = NULL;
            quick_sna_size = 0;
        }
    } 

    // allocate buffer it not allocated
    if (quick_sna_buffer == NULL)
    {
        uint32_t requested_sna_size = 0;
        if (Config::getArch() == "48K")
            requested_sna_size = SNA_48K_SIZE;
        else
            requested_sna_size = SNA_128K_SIZE2;

        #ifdef BOARD_HAS_PSRAM
            quick_sna_buffer = (uint8_t*)ps_calloc(1, requested_sna_size);
        #else
            quick_sna_buffer = (uint8_t*)calloc(1, requested_sna_size);
        #endif
        if (quick_sna_buffer == NULL) {
            Serial.printf("FileSNA::saveQuick: cannot allocate %d bytes for snapshot buffer\n", requested_sna_size);
            KB_INT_START;
            return false;
        }
        quick_sna_size = requested_sna_size;
        Serial.printf("saveQuick: allocated %d bytes for snapshot buffer\n", quick_sna_size);
    }

    bool result = saveToMem(quick_sna_buffer, quick_sna_size);
    KB_INT_START;
    return result;
}

bool FileSNA::saveToMem(uint8_t* dstBuffer, uint32_t size)
{
    uint8_t* snaptr = dstBuffer;

    // write registers: begin with I
    writeByteMem(Z80_GET_I(), snaptr);

    writeWordMemLE(Z80_GET_HLx(), snaptr);
    writeWordMemLE(Z80_GET_DEx(), snaptr);
    writeWordMemLE(Z80_GET_BCx(), snaptr);
    writeWordMemLE(Z80_GET_AFx(), snaptr);

    writeWordMemLE(Z80_GET_HL(), snaptr);
    writeWordMemLE(Z80_GET_DE(), snaptr);
    writeWordMemLE(Z80_GET_BC(), snaptr);

    writeWordMemLE(Z80_GET_IY(), snaptr);
    writeWordMemLE(Z80_GET_IX(), snaptr);

    uint8_t inter = Z80_GET_IFF2() ? 0x04 : 0;
    writeByteMem(inter, snaptr);
    writeByteMem(Z80_GET_R(), snaptr);

    writeWordMemLE(Z80_GET_AF(), snaptr);

    uint16_t SP = Z80_GET_SP();
    if (Config::getArch() == "48K") {
        // decrement stack pointer it for pushing PC to stack, only on 48K
        SP -= 2;
        Mem::writeword(SP, Z80_GET_PC());
    }
    writeWordMemLE(SP, snaptr);

    writeByteMem(Z80_GET_IM(), snaptr);
    uint8_t bordercol = ESPectrum::borderColor;
    writeByteMem(bordercol, snaptr);

    // write RAM pages in 48K address space (0x4000 - 0xFFFF)
    uint8_t pages[3] = {5, 2, 0};
    if (Config::getArch() == "128K")
        pages[2] = Mem::bankLatch;

    for (uint8_t ipage = 0; ipage < 3; ipage++) {
        uint8_t page = pages[ipage];
        writeBlockMem(Mem::ram[page], snaptr, MEM_PG_SZ);
    }

    if (Config::getArch() == "48K")
    {
        // nothing to do here
    }
    else if (Config::getArch() == "128K")
    {
        // write pc
        writeWordMemLE(Z80_GET_PC(), snaptr);

        // write mem bank control port
        uint8_t tmp_port = Mem::bankLatch;
        bitWrite(tmp_port, 3, Mem::videoLatch);
        bitWrite(tmp_port, 4, Mem::romLatch);
        bitWrite(tmp_port, 5, Mem::pagingLock);
        writeByteMem(tmp_port, snaptr);

        writeByteMem(0, snaptr);     // TR-DOS not paged

        // write remaining ram pages
        for (int page = 0; page < 8; page++) {
            if (page != Mem::bankLatch && page != 2 && page != 5) {
                writeBlockMem(Mem::ram[page], snaptr, MEM_PG_SZ);
            }
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool FileSNA::loadQuick()
{
    KB_INT_STOP;

    if (NULL == quick_sna_buffer) {
        // nothing to read
        Serial.println("FileSNA::loadQuick(): nothing to load");
        KB_INT_START;
        return false;
    }

    bool result = loadFromMem(quick_sna_buffer, quick_sna_size);
    KB_INT_START;
    return result;
}

///////////////////////////////////////////////////////////////////////////////

bool FileSNA::loadFromMem(uint8_t* srcBuffer, uint32_t size)
{
    uint8_t* snaptr = srcBuffer;

    String snapshotArch = "48K";

    Mem::bankLatch = 0;
    Mem::pagingLock = 1;
    Mem::videoLatch = 0;
    Mem::romLatch = 0;
    Mem::romInUse = 0;

    Z80_SET_I(readByteMem(snaptr));

    Z80_SET_HLx(readWordMemLE(snaptr));
    Z80_SET_DEx(readWordMemLE(snaptr));
    Z80_SET_BCx(readWordMemLE(snaptr));
    Z80_SET_AFx(readWordMemLE(snaptr));

    Z80_SET_HL(readWordMemLE(snaptr));
    Z80_SET_DE(readWordMemLE(snaptr));
    Z80_SET_BC(readWordMemLE(snaptr));

    Z80_SET_IY(readWordMemLE(snaptr));
    Z80_SET_IX(readWordMemLE(snaptr));

    uint8_t inter = readByteMem(snaptr);
    Z80_SET_IFF2((inter & 0x04) ? true : false);
    Z80_SET_IFF1(Z80_GET_IFF2());
    Z80_SET_R(readByteMem(snaptr));

    Z80_SET_AF(readWordMemLE(snaptr));
    Z80_SET_SP(readWordMemLE(snaptr));

    Z80_SET_IM(readByteMem(snaptr));

    ESPectrum::borderColor = readByteMem(snaptr);

    // read 48K memory
    readBlockMem(snaptr, Mem::ram[5], MEM_PG_SZ);
    readBlockMem(snaptr, Mem::ram[2], MEM_PG_SZ);
    readBlockMem(snaptr, Mem::ram[0], MEM_PG_SZ);

    if (size == SNA_48K_SIZE)
    {
        snapshotArch = "48K";

        // in 48K mode, pop PC from stack
        uint16_t SP = Z80_GET_SP();
        Z80_SET_PC(Mem::readword(SP));
        Z80_SET_SP(SP + 2);
    }
    else
    {
        snapshotArch = "128K";

        // in 128K mode, recover stored PC
        Z80_SET_PC(readWordMemLE(snaptr));

        // tmp_port contains page switching status, including current page number (latch)
        uint8_t tmp_port = readByteMem(snaptr);
        uint8_t tmp_latch = tmp_port & 0x07;

        // copy what was read into page 0 to correct page
        memcpy(Mem::ram[tmp_latch], Mem::ram[0], 0x4000);

        uint8_t tr_dos = readByteMem(snaptr);     // unused
        
        // read remaining pages
        for (int page = 0; page < 8; page++) {
            if (page != tmp_latch && page != 2 && page != 5) {
                readBlockMem(snaptr, Mem::ram[page], 0x4000);
            }
        }

        // decode tmp_port
        Mem::videoLatch = bitRead(tmp_port, 3);
        Mem::romLatch = bitRead(tmp_port, 4);
        Mem::pagingLock = bitRead(tmp_port, 5);
        Mem::bankLatch = tmp_latch;
        Mem::romInUse = Mem::romLatch;
    }

    // just architecturey things
    if (Config::getArch() == "128K")
    {
        if (snapshotArch == "48K")
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
        if (snapshotArch == "128K")
        {
            Config::requestMachine("128K", "SINCLAIR", true);
            Mem::romInUse = 1;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////

