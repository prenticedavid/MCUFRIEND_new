//#define SUPPORT_0139   //costs about 238 bytes
//#define SUPPORT_1289    //costs about 408 bytes
#define SUPPORT_1963    //only works with 16BIT bus anyway
//#define SUPPORT_4532          //costs about 128 bytes.  thanks Leodino
//#define SUPPORT_4535
#define SUPPORT_68140           //very untested
//#define SUPPORT_8347D           //costs about 472 bytes, 0.27s
//#define SUPPORT_8347A           //costs about +178 bytes on top of 8347D
//#define SUPPORT_8352A           //costs about 688 bytes, 0.27s
#define OFFSET_9327 32             //costs about 103 bytes, 0.08s

#include "MCUFRIEND_kbv.h"
#if defined(USE_SERIAL)
#include "mcufriend_serial.h"
 //uint8_t running;
#elif defined(__MBED__)
#include "mcufriend_mbed.h"
#elif defined(__CC_ARM)
#include "mcufriend_keil.h"
#else
#include "mcufriend_shield.h"
#endif

#define MIPI_DCS_REV1   (1<<0)
#define AUTO_READINC    (1<<1)
#define READ_BGR        (1<<2)
#define READ_LOWHIGH    (1<<3)
#define READ_24BITS     (1<<4)
#define XSA_XEA_16BIT   (1<<5)
#define READ_NODUMMY    (1<<6)
#define INVERT_GS       (1<<8)
#define INVERT_SS       (1<<9)
#define MV_AXIS         (1<<10)
#define INVERT_RGB      (1<<11)
#define REV_SCREEN      (1<<12)
#define FLIP_VERT       (1<<13)
#define FLIP_HORIZ      (1<<14)

#if (defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) || defined(__SAM3X8E__))\
 && (defined(USE_MEGA_16BIT_SHIELD) || defined(USE_DUE_16BIT_SHIELD))
#define USING_16BIT_BUS 1
#else
#define USING_16BIT_BUS 0
#endif

#if defined USE_GFX_KBV
MCUFRIEND_kbv::MCUFRIEND_kbv():Adafruit_GFX(240, 320)
{
    // we can not access GPIO pins until AHB has been enabled.
}
#else
MCUFRIEND_kbv::MCUFRIEND_kbv(int CS, int RS, int WR, int RD, int RST):Adafruit_GFX(240, 320)
{
    // we can not access GPIO pins until AHB has been enabled.
}
#endif

static uint8_t done_reset, is8347;

void MCUFRIEND_kbv::reset(void)
{
    done_reset = 1;
    setWriteDir();
    CTL_INIT();
    CS_IDLE;
    RD_IDLE;
    WR_IDLE;
    RESET_IDLE;
    delay(50);
    RESET_ACTIVE;
    delay(100);
    RESET_IDLE;
    delay(100);
	WriteCmdData(0xB0, 0x0000);   //R61520 needs this to read ID
}

void MCUFRIEND_kbv::WriteCmdData(uint16_t cmd, uint16_t dat)
{
    CS_ACTIVE;
    WriteCmd(cmd);
    WriteData(dat);
    CS_IDLE;
}

static void WriteCmdParamN(uint16_t cmd, int8_t N, uint8_t * block)
{
    CS_ACTIVE;
    WriteCmd(cmd);
    while (N-- > 0) {
        uint8_t u8 = *block++;
        CD_DATA;
        write8(u8);
        if (N && is8347) {
            cmd++;
            WriteCmd(cmd);
        }
    }
    CS_IDLE;
}

static inline void WriteCmdParam4(uint8_t cmd, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4)
{
    uint8_t d[4];
    d[0] = d1, d[1] = d2, d[2] = d3, d[3] = d4;
    WriteCmdParamN(cmd, 4, d);
}

//#define WriteCmdParam4(cmd, d1, d2, d3, d4) {uint8_t d[4];d[0] = d1, d[1] = d2, d[2] = d3, d[3] = d4;WriteCmdParamN(cmd, 4, d);}
void MCUFRIEND_kbv::pushCommand(uint16_t cmd, uint8_t * block, int8_t N) { WriteCmdParamN(cmd, N, block); }

static uint16_t read16bits(void)
{
    uint16_t ret;
    uint8_t lo;
#if USING_16BIT_BUS
    READ_16(ret);               //single strobe to read whole bus
    if (ret > 255)              //ID might say 0x00D3
        return ret;
#else
    READ_8(ret);
#endif
    //all MIPI_DCS_REV1 style params are 8-bit
    READ_8(lo);
    return (ret << 8) | lo;
}

uint32_t readReg40(uint16_t reg)
{
    uint16_t h, m, l;
    CS_ACTIVE;
    WriteCmd(reg);
    setReadDir();
    CD_DATA;
    h = read16bits();
    m = read16bits();
    l = read16bits();
    RD_IDLE;
    CS_IDLE;
    setWriteDir();
    return ((uint32_t) h << 24) | (m << 8) | (l >> 8);
}

uint16_t MCUFRIEND_kbv::readReg(uint16_t reg)
{
    uint16_t ret;
    uint8_t lo;
    if (!done_reset)
        reset();
    CS_ACTIVE;
    WriteCmd(reg);
    setReadDir();
    CD_DATA;
    //    READ_16(ret);
    ret = read16bits();
    RD_IDLE;
    CS_IDLE;
    setWriteDir();
    return ret;
}

uint32_t MCUFRIEND_kbv::readReg32(uint16_t reg)
{
    uint16_t h, l;
    CS_ACTIVE;
    WriteCmd(reg);
    setReadDir();
    CD_DATA;
    h = read16bits();
    l = read16bits();
    RD_IDLE;
    CS_IDLE;
    setWriteDir();
    return ((uint32_t) h << 16) | (l);
}

uint16_t MCUFRIEND_kbv::readID(void)
{
    uint16_t ret, ret2;
    uint8_t msb;
    ret = readReg(0);           //forces a reset() if called before begin()
    if (ret == 0x5408)          //the SPFD5408 fails the 0xD3D3 test.
        return 0x5408;
    if (ret == 0x5420)          //the SPFD5420 fails the 0xD3D3 test.
        return 0x5420;
    if (ret == 0x8989)          //SSD1289 is always 8989
        return 0x1289;
    ret = readReg(0x67);        //HX8347-A
    if (ret == 0x4747)
        return 0x8347;
#if defined(SUPPORT_1963) && USING_16BIT_BUS 
    ret = readReg32(0xA1);      //SSD1963: [01 57 61 01]
    if (ret == 0x6101)
        return 0x1963;
#endif
	ret = readReg40(0xBF);
                                //HX8357B: [xx 01 62 83 57 FF] unsupported
	if (ret == 0x9481)          //ILI9481: [xx 02 04 94 81 FF]
        return 0x9481;
    if (ret == 0x1511)          //?R61511: [xx 02 04 15 11] not tested yet
        return 0x1511;
    if (ret == 0x1520)          //?R61520: [xx 01 22 15 20]
        return 0x1520;
    if (ret == 0x1581)          //R61581:  [xx 01 22 15 81]
        return 0x1581;
    if (ret == 0x1400)          //?RM68140:[xx FF 68 14 00] not tested yet
        return 0x6814;
    ret = readReg40(0xEF);      //ILI9327: [xx 02 04 93 27 FF] 
    if (ret == 0x9327)
        return 0x9327;
    uint32_t ret32 = readReg32(0x04);
    msb = ret32 >> 16;
    ret = ret32;	
//    if (msb = 0x38 && ret == 0x8000) //unknown [xx 38 80 00] with D3 = 0x1602
    if (msb == 0x00 && ret == 0x8000) //HX8357-D [xx 00 80 00]
        return 0x8357;
    if (ret == 0x8552)          //ST7789V: [xx 85 85 52]
        return 0x7789;
    if (ret == 0xAC11)          //?unknown [xx 61 AC 11]
        return 0xAC11;
    ret = readReg32(0xD3);      //for ILI9488, 9486, 9340, 9341
    msb = ret >> 8;
    if (msb == 0x93 || msb == 0x94 || msb == 0x77 || msb == 0x16)
        return ret;             //0x9488, 9486, 9340, 9341, 7796
    if (ret == 0x00D3 || ret == 0xD3D3)
        return ret;             //16-bit write-only bus
/*
	msb = 0x12;                 //read 3rd,4th byte.  does not work in parallel
	pushCommand(0xD9, &msb, 1);
	ret2 = readReg(0xD3);
    msb = 0x13;
	pushCommand(0xD9, &msb, 1);
	ret = (ret2 << 8) | readReg(0xD3);
//	if (ret2 == 0x93)
    	return ret2;
*/
	return readReg(0);          //0154, 7783, 9320, 9325, 9335, B505, B509
}

 // independent cursor and window registers.   S6D0154, ST7781 increments.  ILI92320/5 do not.  
int16_t MCUFRIEND_kbv::readGRAM(int16_t x, int16_t y, uint16_t * block, int16_t w, int16_t h)
{
    uint16_t ret, dummy, _MR = _MW;
    int16_t n = w * h, row = 0, col = 0;
    uint8_t r, g, b, tmp;
    if (_lcd_capable & MIPI_DCS_REV1)
        _MR = 0x2E;
    setAddrWindow(x, y, x + w - 1, y + h - 1);
    while (n > 0) {
        if (!(_lcd_capable & MIPI_DCS_REV1)) {
            WriteCmdData(_MC, x + col);
            WriteCmdData(_MP, y + row);
        }
        CS_ACTIVE;
        WriteCmd(_MR);
        setReadDir();
        CD_DATA;
        if (_lcd_capable & READ_NODUMMY) {
            ;
        } else if ((_lcd_capable & MIPI_DCS_REV1) || _lcd_ID == 0x1289) {
            READ_8(r);
        } else {
            READ_16(dummy);
        }
		if (_lcd_ID == 0x1511) READ_8(r);   //extra dummy for R61511
        while (n) {
            if (_lcd_capable & READ_24BITS) {
                READ_8(r);
                READ_8(g);
                READ_8(b);
                if (_lcd_capable & READ_BGR)
                    ret = color565(b, g, r);
                else
                    ret = color565(r, g, b);
            } else {
                READ_16(ret);
                if (_lcd_capable & READ_LOWHIGH)
                    ret = (ret >> 8) | (ret << 8);
                if (_lcd_capable & READ_BGR)
                    ret = (ret & 0x07E0) | (ret >> 11) | (ret << 11);
            }
            *block++ = ret;
            n--;
            if (!(_lcd_capable & AUTO_READINC))
                break;
        }
        if (++col >= w) {
            col = 0;
            if (++row >= h)
                row = 0;
        }
        RD_IDLE;
        CS_IDLE;
        setWriteDir();
    }
    if (!(_lcd_capable & MIPI_DCS_REV1))
        setAddrWindow(0, 0, width() - 1, height() - 1);
    return 0;
}

void MCUFRIEND_kbv::setRotation(uint8_t r)
{
    uint16_t GS, SS, ORG, REV = _lcd_rev;
    uint8_t val, d[3];
    rotation = r & 3;           // just perform the operation ourselves on the protected variables
    _width = (rotation & 1) ? HEIGHT : WIDTH;
    _height = (rotation & 1) ? WIDTH : HEIGHT;
    switch (rotation) {
    case 0:                    //PORTRAIT:
        val = 0x48;             //MY=0, MX=1, MV=0, ML=0, BGR=1
        break;
    case 1:                    //LANDSCAPE: 90 degrees
        val = 0x28;             //MY=0, MX=0, MV=1, ML=0, BGR=1
        break;
    case 2:                    //PORTRAIT_REV: 180 degrees
        val = 0x98;             //MY=1, MX=0, MV=0, ML=1, BGR=1
        break;
    case 3:                    //LANDSCAPE_REV: 270 degrees
        val = 0xF8;             //MY=1, MX=1, MV=1, ML=1, BGR=1
        break;
    }
    if (_lcd_capable & INVERT_GS)
        val ^= 0x80;
    if (_lcd_capable & INVERT_SS)
        val ^= 0x40;
    if (_lcd_capable & INVERT_RGB)
        val ^= 0x08;
    if (_lcd_capable & MIPI_DCS_REV1) {
        if (_lcd_ID == 0x6814) {  //.kbv my weird 0x9486 might be 68140
            GS = (val & 0x80) ? (1 << 6) : 0;   //MY
            SS = (val & 0x40) ? (1 << 5) : 0;   //MX
            val &= 0x28;        //keep MV, BGR, MY=0, MX=0, ML=0
            d[0] = 0;
            d[1] = GS | SS | 0x02;      //MY, MX
            d[2] = 0x3B;
            WriteCmdParamN(0xB6, 3, d);
            goto common_MC;
        } else if (_lcd_ID == 0x1963 || _lcd_ID == 0x9481 || _lcd_ID == 0x1511 || _lcd_ID == 0x1581) {
            if (val & 0x80)
                val |= 0x01;    //GS
            if ((val & 0x40))
                val |= 0x02;    //SS
            if (_lcd_ID == 0x1581) { // no Horizontal Flip
                d[0] = (val & 0x40) ? 0x13 : 0x12; //REV | BGR | SS
                WriteCmdParamN(0xC0, 1, d);
            }
            if (_lcd_ID == 0x1963) val &= ~0xC0;
            if (_lcd_ID == 0x9481 || _lcd_ID == 0x1581) val &= ~0xD0;
            if (_lcd_ID == 0x1511) {
                val &= ~0x10;   //remove ML
                val |= 0xC0;    //force penguin 180 rotation
            }
//            val &= (_lcd_ID == 0x1963) ? ~0xC0 : ~0xD0; //MY=0, MX=0 with ML=0 for ILI9481
            goto common_MC;
        } else if (is8347) {
            _MC = 0x02, _MP = 0x06, _MW = 0x22, _SC = 0x02, _EC = 0x04, _SP = 0x06, _EP = 0x08;
			if (_lcd_ID == 0x5252) {
			    val |= 0x02;   //VERT_SCROLLON
				if (val & 0x10) val |= 0x04;   //if (ML) SS=1 kludge mirror in XXX_REV modes
            }
			goto common_BGR;
        }
      common_MC:
        _MC = 0x2A, _MP = 0x2B, _MW = 0x2C, _SC = 0x2A, _EC = 0x2A, _SP = 0x2B, _EP = 0x2B;
      common_BGR:
        WriteCmdParamN(is8347 ? 0x16 : 0x36, 1, &val);
        _lcd_madctl = val;
//	    if (_lcd_ID	== 0x1963) WriteCmdParamN(0x13, 0, NULL);   //NORMAL mode
    }
    // cope with 9320 variants
    else {
        switch (_lcd_ID) {
#ifdef SUPPORT_0139
        case 0x0139:
            _SC = 0x46, _EC = 0x46, _SP = 0x48, _EP = 0x47;
            goto common_S6D;
#endif
        case 0x0154:
            _SC = 0x37, _EC = 0x36, _SP = 0x39, _EP = 0x38;
          common_S6D:
            _MC = 0x20, _MP = 0x21, _MW = 0x22;
            GS = (val & 0x80) ? (1 << 9) : 0;
            SS = (val & 0x40) ? (1 << 8) : 0;
            WriteCmdData(0x01, GS | SS | 0x0028);       // set Driver Output Control
            goto common_ORG;
        case 0x5420:
        case 0x7793:
        case 0x9326:
		case 0xB509:
            _MC = 0x200, _MP = 0x201, _MW = 0x202, _SC = 0x210, _EC = 0x211, _SP = 0x212, _EP = 0x213;
            GS = (val & 0x80) ? (1 << 15) : 0;
			uint16_t NL;
			NL = ((432 / 8) - 1) << 9;
            if (_lcd_ID == 0x9326 || _lcd_ID == 0x5420) NL >>= 1;
            WriteCmdData(0x400, GS | NL);
            goto common_SS;
        default:
            _MC = 0x20, _MP = 0x21, _MW = 0x22, _SC = 0x50, _EC = 0x51, _SP = 0x52, _EP = 0x53;
            GS = (val & 0x80) ? (1 << 15) : 0;
            WriteCmdData(0x60, GS | 0x2700);    // Gate Scan Line (0xA700)
          common_SS:
            SS = (val & 0x40) ? (1 << 8) : 0;
            WriteCmdData(0x01, SS);     // set Driver Output Control
          common_ORG:
            ORG = (val & 0x20) ? (1 << 3) : 0;
            if (val & 0x08)
                ORG |= 0x1000;  //BGR
            _lcd_madctl = ORG | 0x0030;
            WriteCmdData(0x03, _lcd_madctl);    // set GRAM write direction and BGR=1.
            break;
#ifdef SUPPORT_1289
        case 0x1289:
            _MC = 0x4E, _MP = 0x4F, _MW = 0x22, _SC = 0x44, _EC = 0x44, _SP = 0x45, _EP = 0x46;
            if (rotation & 1)
                val ^= 0xD0;    // exchange Landscape modes
            GS = (val & 0x80) ? (1 << 14) | (1 << 12) : 0;      //called TB (top-bottom)
            SS = (val & 0x40) ? (1 << 9) : 0;   //called RL (right-left)
            ORG = (val & 0x20) ? (1 << 3) : 0;  //called AM
            _lcd_drivOut = GS | SS | (REV << 13) | 0x013F;      //REV=0, BGR=0, MUX=319
            if (val & 0x08)
                _lcd_drivOut |= 0x0800; //BGR
            WriteCmdData(0x01, _lcd_drivOut);   // set Driver Output Control
            WriteCmdData(0x11, ORG | 0x6070);   // set GRAM write direction.
            break;
#endif
		}
    }
    if ((rotation & 1) && ((_lcd_capable & MV_AXIS) == 0)) {
        uint16_t x;
        x = _MC, _MC = _MP, _MP = x;
        x = _SC, _SC = _SP, _SP = x;    //.kbv check 0139
        x = _EC, _EC = _EP, _EP = x;    //.kbv check 0139
    }
    setAddrWindow(0, 0, width() - 1, height() - 1);
    vertScroll(0, HEIGHT, 0);   //reset scrolling after a rotation
}

void MCUFRIEND_kbv::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    // MCUFRIEND just plots at edge if you try to write outside of the box:
    if (x < 0 || y < 0 || x >= width() || y >= height())
        return;
#if defined(OFFSET_9327)
	if (_lcd_ID == 0x9327) {
	    if (rotation == 2) y += OFFSET_9327;
	    if (rotation == 3) x += OFFSET_9327;
    }
#endif
    if (_lcd_capable & MIPI_DCS_REV1) {
        WriteCmdParam4(_MC, x >> 8, x, x >> 8, x);
        WriteCmdParam4(_MP, y >> 8, y, y >> 8, y);
    } else {
        WriteCmdData(_MC, x);
        WriteCmdData(_MP, y);
    }
    WriteCmdData(_MW, color);
}

void MCUFRIEND_kbv::setAddrWindow(int16_t x, int16_t y, int16_t x1, int16_t y1)
{
#if defined(OFFSET_9327)
	if (_lcd_ID == 0x9327) {
	    if (rotation == 2) y += OFFSET_9327, y1 += OFFSET_9327;
	    if (rotation == 3) x += OFFSET_9327, x1 += OFFSET_9327;
    }
#endif
    if (_lcd_capable & MIPI_DCS_REV1) {
        WriteCmdParam4(_MC, x >> 8, x, x1 >> 8, x1);
        WriteCmdParam4(_MP, y >> 8, y, y1 >> 8, y1);
    } else {
        WriteCmdData(_MC, x);
        WriteCmdData(_MP, y);
        if (_lcd_capable & XSA_XEA_16BIT) {
            if (rotation & 1)
                y1 = y = (y1 << 8) | y;
            else
                x1 = x = (x1 << 8) | x;
        }
        WriteCmdData(_SC, x);
        WriteCmdData(_SP, y);
        WriteCmdData(_EC, x1);
        WriteCmdData(_EP, y1);
    }
}

void MCUFRIEND_kbv::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    int16_t end;
    if (w < 0) {
        w = -w;
        x -= w;
    }                           //+ve w
    end = x + w;
    if (x < 0)
        x = 0;
    if (end > width())
        end = width();
    w = end - x;
    if (h < 0) {
        h = -h;
        y -= h;
    }                           //+ve h
    end = y + h;
    if (y < 0)
        y = 0;
    if (end > height())
        end = height();
    h = end - y;

    setAddrWindow(x, y, x + w - 1, y + h - 1);
    CS_ACTIVE;
    WriteCmd(_MW);
    if (h > w) {
        end = h;
        h = w;
        w = end;
    }
    uint8_t hi = color >> 8, lo = color & 0xFF;
    CD_DATA;
    while (h-- > 0) {
        end = w;
#if USING_16BIT_BUS
#if defined(__SAM3X8E__)
#define STROBE_16BIT {WR_ACTIVE;WR_ACTIVE;WR_ACTIVE;WR_IDLE;WR_IDLE;}
#else
#define STROBE_16BIT {WR_ACTIVE; WR_IDLE;}
#endif
        write_16(color);        //we could just do the strobe
        lo = end & 7;
        hi = end >> 3;
        if (hi)
            do {
                STROBE_16BIT;
                STROBE_16BIT;
                STROBE_16BIT;
                STROBE_16BIT;
                STROBE_16BIT;
                STROBE_16BIT;
                STROBE_16BIT;
                STROBE_16BIT;
            } while (--hi > 0);
        while (lo-- > 0) {
            STROBE_16BIT;
        }
#else
        do {
            write8(hi);
            write8(lo);
        } while (--end != 0);
#endif
    }
    CS_IDLE;
    if (!(_lcd_capable & MIPI_DCS_REV1))
        setAddrWindow(0, 0, width() - 1, height() - 1);
}

void MCUFRIEND_kbv::pushColors(uint16_t * block, int16_t n, bool first)
{
    uint16_t color;
    CS_ACTIVE;
    if (first) {
        WriteCmd(_MW);
    }
    CD_DATA;
    while (n-- > 0) {
        color = *block++;
        write16(color);
    }
    CS_IDLE;
}

void MCUFRIEND_kbv::pushColors(uint8_t * block, int16_t n, bool first)
{
    uint16_t color;
    uint8_t h, l;
    CS_ACTIVE;
    if (first) {
        WriteCmd(_MW);
    }
    CD_DATA;
    while (n-- > 0) {
        h = (*block++);
        l = (*block++);
        color = h << 8 | l;
        write16(color);
    }
    CS_IDLE;
}

void MCUFRIEND_kbv::pushColors(const uint8_t * block, int16_t n, bool first)
{
    uint16_t color;
    uint8_t h, l;
    CS_ACTIVE;
    if (first) {
        WriteCmd(_MW);
    }
    CD_DATA;
    while (n-- > 0) {
        l = pgm_read_byte(block++);
        h = pgm_read_byte(block++);
        color = h << 8 | l;
        write16(color);
    }
    CS_IDLE;
}

void MCUFRIEND_kbv::vertScroll(int16_t top, int16_t scrollines, int16_t offset)
{
#if defined(OFFSET_9327)
	if (_lcd_ID == 0x9327) {
	    if (rotation == 2 || rotation == 3) top += OFFSET_9327;
    }
#endif
    int16_t bfa = HEIGHT - top - scrollines;  // bottom fixed area
    int16_t vsp;
    int16_t sea = top;
	if (_lcd_ID == 0x9327) bfa += 32;
    if (offset <= -scrollines || offset >= scrollines) offset = 0; //valid scroll
	vsp = top + offset; // vertical start position
    if (offset < 0)
        vsp += scrollines;          //keep in unsigned range
    sea = top + scrollines - 1;
    if (_lcd_capable & MIPI_DCS_REV1) {
        uint8_t d[6];           // for multi-byte parameters
/*
        if (_lcd_ID == 0x9327) {        //panel is wired for 240x432 
            if (rotation == 2 || rotation == 3) { //180 or 270 degrees
                if (scrollines == HEIGHT) {
                    scrollines = 432;   // we get a glitch but hey-ho
                    vsp -= 432 - HEIGHT;
                }
                if (vsp < 0)
                    vsp += 432;
            }
            bfa = 432 - top - scrollines;
        }
*/
        d[0] = top >> 8;        //TFA
        d[1] = top;
        d[2] = scrollines >> 8; //VSA
        d[3] = scrollines;
        d[4] = bfa >> 8;        //BFA
        d[5] = bfa;
        WriteCmdParamN(is8347 ? 0x0E : 0x33, 6, d);
//        if (offset == 0 && rotation > 1) vsp = top + scrollines;   //make non-valid
		d[0] = vsp >> 8;        //VSP
        d[1] = vsp;
        WriteCmdParamN(is8347 ? 0x14 : 0x37, 2, d);
		if (is8347) { 
		    d[0] = (offset != 0) ? (_lcd_ID == 0x8347 ? 0x02 : 0x08) : 0;
			WriteCmdParamN(_lcd_ID == 0x8347 ? 0x18 : 0x01, 1, d);  //HX8347-D
		} else if (offset == 0 && (_lcd_capable & MIPI_DCS_REV1)) {
			WriteCmdParamN(0x13, 0, NULL);    //NORMAL i.e. disable scroll
		}
		return;
    }
    // cope with 9320 style variants:
    switch (_lcd_ID) {
    case 0x7783:
        WriteCmdData(0x61, _lcd_rev);   //!NDL, !VLE, REV
        WriteCmdData(0x6A, vsp);        //VL#
        break;
#ifdef SUPPORT_0139
    case 0x0139:
        WriteCmdData(0x41, sea);        //SEA
        WriteCmdData(0x42, top);        //SSA
        WriteCmdData(0x43, vsp - top);  //SST
        break;
#endif
    case 0x0154:
        WriteCmdData(0x31, sea);        //SEA
        WriteCmdData(0x32, top);        //SSA
        WriteCmdData(0x33, vsp - top);  //SST
        break;
#ifdef SUPPORT_1289
    case 0x1289:
        WriteCmdData(0x41, vsp);        //VL#
        break;
#endif
	case 0x5420:
    case 0x7793:
	case 0x9326:
	case 0xB509:
        WriteCmdData(0x401, (1 << 1) | _lcd_rev);       //VLE, REV 
        WriteCmdData(0x404, vsp);       //VL# 
        break;
    default:
        // 0x6809, 0x9320, 0x9325, 0x9335, 0xB505 can only scroll whole screen
        WriteCmdData(0x61, (1 << 1) | _lcd_rev);        //!NDL, VLE, REV
        WriteCmdData(0x6A, vsp);        //VL#
        break;
    }
}

void MCUFRIEND_kbv::invertDisplay(boolean i)
{
    uint8_t val;
    _lcd_rev = ((_lcd_capable & REV_SCREEN) != 0) ^ i;
    if (_lcd_capable & MIPI_DCS_REV1) {
        if (is8347) {
            // HX8347D: 0x36 Panel Characteristic. REV_Panel
            // HX8347A: 0x36 is Display Control 10
            if (_lcd_ID == 0x8347 || _lcd_ID == 0x5252) // HX8347-A, HX5352-A
			    val = _lcd_rev ? 6 : 2;       //INVON id bit#2,  NORON=bit#1
            else val = _lcd_rev ? 8 : 10;     //HX8347-D, G, I: SCROLLON=bit3, INVON=bit1
            // HX8347: 0x01 Display Mode has diff bit mapping for A, D 
            WriteCmdParamN(0x01, 1, &val);
        } else
            WriteCmdParamN(_lcd_rev ? 0x21 : 0x20, 0, NULL);
        return;
    }
    // cope with 9320 style variants:
    switch (_lcd_ID) {
#ifdef SUPPORT_0139
    case 0x0139:
#endif
    case 0x0154:
        WriteCmdData(0x07, 0x13 | (_lcd_rev << 2));     //.kbv kludge
        break;
#ifdef SUPPORT_1289
    case 0x1289:
        _lcd_drivOut &= ~(1 << 13);
        if (_lcd_rev)
            _lcd_drivOut |= (1 << 13);
        WriteCmdData(0x01, _lcd_drivOut);
        break;
#endif
	case 0x5420:
    case 0x7793:
    case 0x9326:
	case 0xB509:
        WriteCmdData(0x401, (1 << 1) | _lcd_rev);       //.kbv kludge VLE 
        break;
    default:
        WriteCmdData(0x61, _lcd_rev);
        break;
    }
}

#define TFTLCD_DELAY 0xFFFF
#define TFTLCD_DELAY8 0xFF
static void init_table(const void *table, int16_t size)
{
    uint8_t *p = (uint8_t *) table, dat[16];
    while (size > 0) {
        uint8_t cmd = pgm_read_byte(p++);
        uint8_t len = pgm_read_byte(p++);
        if (cmd == TFTLCD_DELAY8) {
            delay(len);
            len = 0;
        } else {
            for (uint8_t i = 0; i < len; i++)
                dat[i] = pgm_read_byte(p++);
            WriteCmdParamN(cmd, len, dat);
        }
        size -= len + 2;
    }
}

static void init_table16(const void *table, int16_t size)
{
    uint16_t *p = (uint16_t *) table;
    while (size > 0) {
        uint16_t cmd = pgm_read_word(p++);
        uint16_t d = pgm_read_word(p++);
        if (cmd == TFTLCD_DELAY)
            delay(d);
        else {
            CS_ACTIVE;
            WriteCmd(cmd);
            WriteData(d);
            CS_IDLE;
        }
        size -= 2 * sizeof(int16_t);
    }
}

void MCUFRIEND_kbv::begin(uint16_t ID)
{
    int16_t *p16;               //so we can "write" to a const protected variable.
    reset();
    _lcd_xor = 0;
    if (ID == 0)
        ID = 0x9341;
    switch (_lcd_ID = ID) {
/*
	static const uint16_t _regValues[] PROGMEM = {
    0x0000, 0x0001, // start oscillation
    0x0007, 0x0000, //  source output control 0 D0 
    0x0013, 0x0000, // power control 3 off
    0x0011, 0x2604, //    
    0x0014, 0x0015, //   
    0x0010, 0x3C00, //  
 //    0x0013, 0x0040, // 
 //    0x0013, 0x0060, //     
 //    0x0013, 0x0070, // 
    0x0013, 0x0070, // power control 3 PON PON1 AON
       
    0x0001, 0x0127, //      driver output control
 //    0x0002, 0x0700, //  field 0 b/c waveform xor waveform
    0x0003, 0x1030, //    
    0x0007, 0x0000, //    
    0x0008, 0x0404, //    
    0x000B, 0x0200, // 
    0x000C, 0x0000, //   
    0x00015,0x0000, //     
       
    //gamma setting    
    0x0030, 0x0000,      
    0x0031, 0x0606,    
    0x0032, 0x0006,    
    0x0033, 0x0403,  
    0x0034, 0x0107,  
    0x0035, 0x0101, 
    0x0036, 0x0707,   
    0x0037, 0x0304,   
    0x0038, 0x0A00,     
    0x0039, 0x0706,     
       
    0x0040, 0x0000,     
    0x0041, 0x0000,      
    0x0042, 0x013F,    
    0x0043, 0x0000,   
    0x0044, 0x0000,     
    0x0045, 0x0000,     
    0x0046, 0xEF00,    
    0x0047, 0x013F,     
    0x0048, 0x0000,     
    0x0007, 0x0011,  
    0x0007, 0x0017,     
};
*/
#ifdef SUPPORT_0139
    case 0x0139:
        _lcd_capable = AUTO_READINC | REV_SCREEN | XSA_XEA_16BIT;
        static const uint16_t S6D0139_regValues[] PROGMEM = {
            0x0000, 0x0001,     //Start oscillator
            0x0011, 0x1a00,     //Power Control 2
            0x0014, 0x2020,     //Power Control 4
            0x0010, 0x0900,     //Power Control 1
            0x0013, 0x0040,     //Power Control 3
            0x0013, 0x0060,     //Power Control 3
            0x0013, 0x0070,     //Power Control 3
            0x0011, 0x1a04,     //Power Control 2
            0x0010, 0x2f00,     //Power Control 1
            0x0001, 0x0127,     //Driver Control: SM=0, GS=0, SS=1, 240x320
            0x0002, 0x0100,     //LCD Control:  (.kbv was 0700) FLD=0, BC= 0, EOR=1
            0x0003, 0x1030,     //Entry Mode:    TR1=0, DFM=0, BGR=1, I_D=3   
            0x0007, 0x0000,     //Display Control: everything off
            0x0008, 0x0808,     //Blank Period:  FP=98, BP=8
            0x0009, 0x0000,     //f.k.
            0x000b, 0x0000,     //Frame Control:
            0x000c, 0x0000,     //Interface Control: system i/f
            0x0040, 0x0000,     //Scan Line
            0x0041, 0x0000,     //Vertical Scroll Control
            0x0042, 0x013f,     //Screen 1 End
            0x0043, 0x0000,     //Screen 1 start
            0x0044, 0x00ef,     //Screen 2 end
            0x0045, 0x0000,     //Screen 2 start
            0x0046, 0xef00,     //Horiz address H=end, L=start
            0x0047, 0x013f,     //Vert end
            0x0048, 0x0000,     //Vert start
            0x0007, 0x0014,     //Display Control: SPT=1, REV=1
            0x0007, 0x0016,     //Display Control: SPT=1, REV=1, display on
            0x0007, 0x0017,     //Display Control: SPT=1, REV=1, display on, GON
        };
        init_table16(S6D0139_regValues, sizeof(S6D0139_regValues));
        break;
#endif
    case 0x0154:
        _lcd_capable = AUTO_READINC | REV_SCREEN;
        static const uint16_t S6D0154_regValues[] PROGMEM = {
            0x0011, 0x001A,
            0x0012, 0x3121,
            0x0013, 0x006C,
            0x0014, 0x4249,

            0x0010, 0x0800,
            TFTLCD_DELAY, 10,
            0x0011, 0x011A,
            TFTLCD_DELAY, 10,
            0x0011, 0x031A,
            TFTLCD_DELAY, 10,
            0x0011, 0x071A,
            TFTLCD_DELAY, 10,
            0x0011, 0x0F1A,
            TFTLCD_DELAY, 10,
            0x0011, 0x0F3A,
            TFTLCD_DELAY, 30,

            0x0001, 0x0128,
            0x0002, 0x0100,
            0x0003, 0x1030,
            0x0007, 0x1012,
            0x0008, 0x0303,
            0x000B, 0x1100,
            0x000C, 0x0000,
            0x000F, 0x1801,
            0x0015, 0x0020,
            /*
               0x0050,0x0101,
               0x0051,0x0603,
               0x0052,0x0408,
               0x0053,0x0000,
               0x0054,0x0605,
               0x0055,0x0406,
               0x0056,0x0303,
               0x0057,0x0303,
               0x0058,0x0010,
               0x0059,0x1000,
             */
            0x0007, 0x0012,
            TFTLCD_DELAY, 40,
            0x0007, 0x0013,     /* GRAM Address Set */
            0x0007, 0x0017,     /* Display Control DISPLAY ON */

            0x0036, 0x00EF,
            0x0037, 0x0000,
            0x0038, 0x013F,
            0x0039, 0x0000,
        };
        init_table16(S6D0154_regValues, sizeof(S6D0154_regValues));

        break;

#ifdef SUPPORT_1289
    case 0x1289:
        _lcd_capable = 0 | XSA_XEA_16BIT | REV_SCREEN;
        // came from MikroElektronika library http://www.hmsprojects.com/tft_lcd.html
        static const uint16_t SSD1289_regValues[] PROGMEM = {
            0x0000, 0x0001,
            0x0003, 0xA8A4,
            0x000C, 0x0000,
            0x000D, 0x080C,     // was 0x800C
            0x000E, 0x2B00,
            0x001E, 0x00B7,
            0x0001, 0x2B3F,     // was 0x2B3F,
            0x0002, 0x0400,     // was 0x0600
            0x0010, 0x0000,
            0x0011, 0x6070,     // was 0x6070
            0x0005, 0x0000,
            0x0006, 0x0000,
            0x0016, 0xEF1C,
            0x0017, 0x0003,
            0x0007, 0x0233,
            0x000B, 0x0000,
            0x000F, 0x0000,
            0x0041, 0x0000,
            0x0042, 0x0000,
            0x0048, 0x0000,
            0x0049, 0x013F,
            0x004A, 0x0000,
            0x004B, 0x0000,
            0x0044, 0xEF95,
            0x0045, 0x0000,
            0x0046, 0x013F,
            0x0030, 0x0707,
            0x0031, 0x0204,
            0x0032, 0x0204,
            0x0033, 0x0502,
            0x0034, 0x0507,
            0x0035, 0x0204,
            0x0036, 0x0204,
            0x0037, 0x0502,
            0x003A, 0x0302,
            0x003B, 0x0302,
            0x0023, 0x0000,
            0x0024, 0x0000,
            0x0025, 0x8000,
            0x004f, 0x0000,
            0x004e, 0x0000,
        };
        init_table16(SSD1289_regValues, sizeof(SSD1289_regValues));
        break;
#endif

    case 0x1511:                // Unknown from Levy
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1;   //extra read_8(dummy)
        static const uint8_t R61511_regValues[] PROGMEM = {
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 120*2,  // .kbv will power up with ONLY reset, sleep out, display on
            0x28, 0,            //Display Off
			0xB0, 1, 0x00,       //Command Access Protect
			0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 150,
            0x29, 0,            //Display On
            0x3A, 1, 0x55,      //Pixel read=565, write=565
        };
        init_table(R61511_regValues, sizeof(R61511_regValues));
        p16 = (int16_t *) & HEIGHT;
        *p16 = 480;
        p16 = (int16_t *) & WIDTH;
        *p16 = 320;
        break;

    case 0x1520:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | READ_24BITS;
        static const uint8_t R61526_regValues[] PROGMEM = {
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 120*2,  // .kbv will power up with ONLY reset, sleep out, display on
            0x28, 0,            //Display Off
//			0xB0, 2, 0x00, 0x00,      //Command Access Protect
//			0xC0, 8, 0x0A, 0x4F, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00,  //DM=1, BGR=1
			0xB0, 1, 0x00,       //Command Access Protect
			0xC0, 1, 0x0A,      //DM=1, BGR=1
			0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 150,
            0x29, 0,            //Display On
            0x3A, 1, 0x55,      //Pixel read=565, write=565
        };
        init_table(R61526_regValues, sizeof(R61526_regValues));
        break;

#if defined(SUPPORT_1963) && USING_16BIT_BUS 
	case 0x1963:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | READ_NODUMMY | INVERT_SS | INVERT_RGB;
        // from NHD 5.0" 8-bit
        static const uint8_t SSD1963_NHD_50_regValues[] PROGMEM = {
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 100,
            (0xE0), 1, 0x01,    // PLL enable
            TFTLCD_DELAY8, 10,
            (0xE0), 1, 0x03,    // Lock PLL
            (0xB0), 7, 0x08, 0x80, 0x03, 0x1F, 0x01, 0xDF, 0x00,        //LCD SPECIFICATION
            (0xF0), 1, 0x03,    //was 00 pixel data interface
            //            (0x3A), 1, 0x60,        // SET R G B format = 6 6 6
            (0xE2), 3, 0x1D, 0x02, 0x54,        //PLL multiplier, set PLL clock to 120M
            (0xE6), 3, 0x02, 0xFF, 0xFF,        //PLL setting for PCLK, depends on resolution
            (0xB4), 8, 0x04, 0x20, 0x00, 0x58, 0x80, 0x00, 0x00, 0x00,  //HSYNC
            (0xB6), 7, 0x02, 0x0D, 0x00, 0x20, 0x01, 0x00, 0x00,        //VSYNC
            (0x13), 0,          //Enter Normal mode
            (0x38), 0,          //Exit Idle mode
            0x29, 0,            //Display On
        };
        // from NHD 7.0" 8-bit
        static const uint8_t SSD1963_NHD_70_regValues[] PROGMEM = {
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 120,
            (0xE2), 3, 0x1D, 0x02, 0x04,        //PLL multiplier, set PLL clock to 120M
            (0xE0), 1, 0x01,    // PLL enable
            TFTLCD_DELAY8, 10,
            (0xE0), 1, 0x03,    // Lock PLL
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 120,
            (0xB0), 7, 0x08, 0x80, 0x03, 0x1F, 0x01, 0xDF, 0x00,        //LCD SPECIFICATION
            (0xF0), 1, 0x03,    //was 00 pixel data interface
            //            (0x3A), 1, 0x60,        // SET R G B format = 6 6 6
            (0xE6), 3, 0x0F, 0xFF, 0xFF,        //PLL setting for PCLK, depends on resolution
            (0xB4), 8, 0x04, 0x20, 0x00, 0x58, 0x80, 0x00, 0x00, 0x00,  //HSYNC
            (0xB6), 7, 0x02, 0x0D, 0x00, 0x20, 0x01, 0x00, 0x00,        //VSYNC
            (0x13), 0,          //Enter Normal mode
            (0x38), 0,          //Exit Idle mode
            0x29, 0,            //Display On
        };
        // from UTFTv2.81 initlcd.h
        static const uint8_t SSD1963_800_regValues[] PROGMEM = {
            (0xE2), 3, 0x1E, 0x02, 0x54,        //PLL multiplier, set PLL clock to 120M
            (0xE0), 1, 0x01,    // PLL enable
            TFTLCD_DELAY8, 10,
            (0xE0), 1, 0x03,    //
            TFTLCD_DELAY8, 10,
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 100,
            (0xE6), 3, 0x03, 0xFF, 0xFF,        //PLL setting for PCLK, depends on resolution
            (0xB0), 7, 0x24, 0x00, 0x03, 0x1F, 0x01, 0xDF, 0x00,        //LCD SPECIFICATION
            //            (0xB0), 7, 0x24, 0x00, 0x03, 0x1F, 0x01, 0xDF, 0x2D,        //LCD SPECIFICATION
            (0xB4), 8, 0x03, 0xA0, 0x00, 0x2E, 0x30, 0x00, 0x0F, 0x00,  //HSYNC
            (0xB6), 7, 0x02, 0x0D, 0x00, 0x10, 0x10, 0x00, 0x08,        //VSYNC
            (0xBA), 1, 0x0F,    //GPIO[3:0] out 1
            (0xB8), 2, 0x07, 0x01,      //GPIO3=input, GPIO[2:0]=output
            (0xF0), 1, 0x03,    //pixel data interface
            TFTLCD_DELAY8, 1,
            0x28, 0,            //Display Off
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 100,
            0x29, 0,            //Display On
            (0xBE), 6, 0x06, 0xF0, 0x01, 0xF0, 0x00, 0x00,      //set PWM for B/L
            (0xD0), 1, 0x0D,
        };
        // from UTFTv2.82 initlcd.h
        static const uint8_t SSD1963_480_regValues[] PROGMEM = {
            (0xE2), 3, 0x23, 0x02, 0x54,        //PLL multiplier, set PLL clock to 120M
            (0xE0), 1, 0x01,    // PLL enable
            TFTLCD_DELAY8, 10,
            (0xE0), 1, 0x03,    //
            TFTLCD_DELAY8, 10,
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 100,
            (0xE6), 3, 0x01, 0x1F, 0xFF,        //PLL setting for PCLK, depends on resolution
            (0xB0), 7, 0x20, 0x00, 0x01, 0xDF, 0x01, 0x0F, 0x00,        //LCD SPECIFICATION
            (0xB4), 8, 0x02, 0x13, 0x00, 0x08, 0x2B, 0x00, 0x02, 0x00,  //HSYNC
            (0xB6), 7, 0x01, 0x20, 0x00, 0x04, 0x0C, 0x00, 0x02,        //VSYNC
            (0xBA), 1, 0x0F,    //GPIO[3:0] out 1
            (0xB8), 2, 0x07, 0x01,      //GPIO3=input, GPIO[2:0]=output
            (0xF0), 1, 0x03,    //pixel data interface
            TFTLCD_DELAY8, 1,
            0x28, 0,            //Display Off
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 100,
            0x29, 0,            //Display On
            (0xBE), 6, 0x06, 0xF0, 0x01, 0xF0, 0x00, 0x00,      //set PWM for B/L
            (0xD0), 1, 0x0D,
        };
//        init_table(SSD1963_480_regValues, sizeof(SSD1963_480_regValues));
        init_table(SSD1963_800_regValues, sizeof(SSD1963_800_regValues));
        p16 = (int16_t *) & HEIGHT;
        *p16 = 480;
        p16 = (int16_t *) & WIDTH;
        *p16 = 800;
        break;
#endif

#ifdef SUPPORT_4532
//Support for LG Electronics LGDP4532 (also 4531 i guess) by Leodino v1.0 2-Nov-2016
//based on data by waveshare and the datasheet of LG Electronics
//My approach to get it working: the parameters by waveshare did no make it function allright
//I started with remming lines to see if this helped. Basically the stuff in range 41-93
//gives problems.
//The other lines that are REMmed give no problems, but it seems default values are OK as well.
case 0x4532:    // thanks Leodino
	_lcd_capable = 0 | REV_SCREEN;  // | INVERT_GS;
	static const uint16_t LGDP4532_regValues[] PROGMEM = {
		0x0000,0x0001, //Device code read
		0x0010,0x0628, //Power control 1 SAP[2:0] BT[3:0] AP[2:0] DK DSTB SLP
		0x0012,0x0006, //Power control 3 PON VRH[3:0]
		//0x0013,0x0A32, //Power control 4 VCOMG VDV[4:0] VCM[6:0]
		0x0011,0x0040, //Power control 2; DC1[2:0] DC0[2:0] VC[2:0]
		//0x0015,0x0050, //Regulator control RSET RI[2:0] RV[2:0] RCONT[2:0]
		0x0012,0x0016, //Power control 3 PON VRH[3:0]
		TFTLCD_DELAY,50,
		0x0010,0x5660, //Power control 1 SAP[2:0] BT[3:0] AP[2:0] DK DSTB SLP
		TFTLCD_DELAY,50,
		//0x0013,0x2A4E, //Power control 4 VCOMG VDV[4:0] VCM[6:0]
		//0x0001,0x0100, //Driver output control SM SS
		//0x0002,0x0300, //LCD Driving Wave Control
		//0x0003,0x1030, //Entry mode TRI DFM  BGR  ORG I/D[1:0] AM
		//0x0007,0x0202, //Display Control 1 PTDE[1:0] BASEE GON DTE COL D[1:0]
		TFTLCD_DELAY,50,
		//0x0008,0x0202, //Display Control 2 FP[3:0] BP[3:0] front and back porch (blank period at begin and end..)
		//0x000A,0x0000, //Test Register 1 (RA0h)
		//Gamma adjustment
		0x0030,0x0000,
		0x0031,0x0402,
		0x0032,0x0106,
		0x0033,0x0700,
		0x0034,0x0104,
		0x0035,0x0301,
		0x0036,0x0707,
		0x0037,0x0305,
		0x0038,0x0208,
		0x0039,0x0F0B,
		TFTLCD_DELAY,50,
		//some of this stuff in range 41-93 really throws things off....
		//0x0041,0x0002,
		//0x0060,0x2700, //Driver Output Control (R60h)
		//0x0061,0x0001, //Base Image Display Control (R61h)
		//0x0090,0x0119,   //Panel Interface Control 1 (R90h) DIVI[1:0]  RTNI[4:0]
		//0x0092,0x010A,  //Panel Interface Control 2 (R92h)  NOWI[2:0] EQI2[1:0] EQI1[1:0]
		//0x0093,0x0004, //Panel Interface Control 3 (R93h) MCPI[2:0]
		//0x00A0,0x0100, //Test Register 1 (RA0h)
		TFTLCD_DELAY,50,
		0x0007,0x0133, //Display Control 1 PTDE[1:0] BASEE GON DTE COL D[1:0]
		TFTLCD_DELAY,50,
		//0x00A0,0x0000, //Test Register 1 (RA0h)
		};
	init_table16(LGDP4532_regValues, sizeof(LGDP4532_regValues));
	break;
#endif

#ifdef SUPPORT_4535
    case 0x4535:
        _lcd_capable = 0 | REV_SCREEN;  // | INVERT_GS;
        static const uint16_t LGDP4535_regValues[] PROGMEM = {
            0x0015, 0x0030,     // Set the internal vcore voltage                                               
            0x009A, 0x0010,     // Start internal OSC 
            0x0011, 0x0020,     // set SS and SM bit 
            0x0010, 0x3428,     // set 1 line inversion 
            0x0012, 0x0002,     // set GRAM write direction and BGR=1  
            0x0013, 0x1038,     // Resize register 
            TFTLCD_DELAY, 40,
            0x0012, 0x0012,     // set the back porch and front porch 
            TFTLCD_DELAY, 40,
            0x0010, 0x3420,     // set non-display area refresh cycle ISC[3:0] 
            0x0013, 0x3045,     // FMARK function 
            TFTLCD_DELAY, 70,
            0x0030, 0x0000,     // RGB interface setting 
            0x0031, 0x0402,     // Frame marker Position 
            0x0032, 0x0307,     // RGB interface polarity 
            0x0033, 0x0304,     // SAP, BT[3:0], AP, DSTB, SLP, STB 
            0x0034, 0x0004,     // DC1[2:0], DC0[2:0], VC[2:0] 
            0x0035, 0x0401,     // VREG1OUT voltage 
            0x0036, 0x0707,     // VDV[4:0] for VCOM amplitude 
            0x0037, 0x0305,     // SAP, BT[3:0], AP, DSTB, SLP, STB 
            0x0038, 0x0610,     // DC1[2:0], DC0[2:0], VC[2:0] 
            0x0039, 0x0610,     // VREG1OUT voltage 
            0x0001, 0x0100,     // VDV[4:0] for VCOM amplitude 
            0x0002, 0x0300,     // VCM[4:0] for VCOMH 
            0x0003, 0x1030,     // GRAM horizontal Address 
            0x0008, 0x0808,     // GRAM Vertical Address 
            0x000A, 0x0008,
            0x0060, 0x2700,     // Gate Scan Line 
            0x0061, 0x0001,     // NDL,VLE, REV 
            0x0090, 0x013E,
            0x0092, 0x0100,
            0x0093, 0x0100,
            0x00A0, 0x3000,
            0x00A3, 0x0010,
            0x0007, 0x0001,
            0x0007, 0x0021,
            0x0007, 0x0023,
            0x0007, 0x0033,
            0x0007, 0x0133,
        };
        init_table16(LGDP4535_regValues, sizeof(LGDP4535_regValues));
        break;
#endif

#ifdef SUPPORT_68140
    case 0x6814:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS;
		static const uint8_t RM68140_regValues_max[] PROGMEM = {        //
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 50,
            0x28, 0,            //Display Off
#if USING_16BIT_BUS
            0x3A, 1, 0x55,      //Pixel format .kbv my Mega Shield
#else
            0x3A, 1, 0x66,      //Pixel format Attila's Uno Shield
#endif
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 150,
            0x29, 0,            //Display On
        };
        init_table(RM68140_regValues_max, sizeof(RM68140_regValues_max));
        p16 = (int16_t *) & HEIGHT;
        *p16 = 480;
        p16 = (int16_t *) & WIDTH;
        *p16 = 320;
        break;
#endif

    case 0x7783:
        _lcd_capable = AUTO_READINC | REV_SCREEN | INVERT_GS;
        static const uint16_t ST7781_regValues[] PROGMEM = {
            0x00FF, 0x0001,     //can we do 0xFF
            0x00F3, 0x0008,
            //  LCD_Write_COM(0x00F3,

            0x00, 0x0001,
            0x0001, 0x0100,     // Driver Output Control Register (R01h)
            0x0002, 0x0700,     // LCD Driving Waveform Control (R02h)
            0x0003, 0x1030,     // Entry Mode (R03h)
            0x0008, 0x0302,
            0x0009, 0x0000,
            0x0010, 0x0000,     // Power Control 1 (R10h)
            0x0011, 0x0007,     // Power Control 2 (R11h)
            0x0012, 0x0000,     // Power Control 3 (R12h)
            0x0013, 0x0000,     // Power Control 4 (R13h)
            TFTLCD_DELAY, 50,
            0x0010, 0x14B0,     // Power Control 1 (R10h)
            TFTLCD_DELAY, 10,
            0x0011, 0x0007,     // Power Control 2 (R11h)
            TFTLCD_DELAY, 10,
            0x0012, 0x008E,     // Power Control 3 (R12h)
            0x0013, 0x0C00,     // Power Control 4 (R13h)
            0x0029, 0x0015,     // NVM read data 2 (R29h)
            TFTLCD_DELAY, 10,
            0x0030, 0x0000,     // Gamma Control 1
            0x0031, 0x0107,     // Gamma Control 2
            0x0032, 0x0000,     // Gamma Control 3
            0x0035, 0x0203,     // Gamma Control 6
            0x0036, 0x0402,     // Gamma Control 7
            0x0037, 0x0000,     // Gamma Control 8
            0x0038, 0x0207,     // Gamma Control 9
            0x0039, 0x0000,     // Gamma Control 10
            0x003C, 0x0203,     // Gamma Control 13
            0x003D, 0x0403,     // Gamma Control 14
            0x0050, 0x0000,     // Window Horizontal RAM Address Start (R50h)
            0x0051, 240 - 1,    // Window Horizontal RAM Address End (R51h)
            0x0052, 0X0000,     // Window Vertical RAM Address Start (R52h)
            0x0053, 320 - 1,    // Window Vertical RAM Address End (R53h)
            0x0060, 0xA700,     // Driver Output Control (R60h) .kbv was 0xa700
            0x0061, 0x0001,     // Driver Output Control (R61h)
            0x0090, 0X0029,     // Panel Interface Control 1 (R90h)

            // Display On
            0x0007, 0x0133,     // Display Control (R07h)
            TFTLCD_DELAY, 50,
        };
        init_table16(ST7781_regValues, sizeof(ST7781_regValues));
        break;
    case 0x7789:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | READ_24BITS;
        static const uint8_t ST7789_regValues[] PROGMEM = {
            0x01, 0,            //Soft Reset
            (0x11), 0,          //exit SLEEP mode
            TFTLCD_DELAY8, 100, // if from Sleep
            (0x36), 1, 0x80,    //MADCTL: memory data access control
            //        (0x3A), 1, 0x66,      //COLMOD: Interface Pixel format *** I use 262K-colors in 18bit/pixel format when using 8-bit interface to allow 3-bytes per pixel
            (0x3A), 1, 0x55,    //COLMOD: Interface Pixel format  *** I use 65K-colors in 16bit/pixel (5-6-5) format when using 16-bit interface to allow 1-byte per pixel
            (0xB2), 5, 0x0C, 0x0C, 0x00, 0x33, 0x33,    //PORCTRK: Porch setting [08 08 00 22 22] PSEN=0 anyway
            (0xB7), 1, 0x35,    //GCTRL: Gate Control [35]
            (0xBB), 1, 0x2B,    //VCOMS: VCOM setting VCOM=1.175 [20] VCOM=0.9
            (0xC0), 1, 0x04,    //LCMCTRL: LCM Control [2C]
            (0xC2), 2, 0x01, 0xFF,      //VDVVRHEN: VDV and VRH Command Enable [01 FF]
            (0xC3), 1, 0x11,    //VRHS: VRH Set VAP=4.4, VAN=-4.4 [0B]
            (0xC4), 1, 0x20,    //VDVS: VDV Set [20]
            (0xC6), 1, 0x0F,    //FRCTRL2: Frame Rate control in normal mode [0F]
            (0xD0), 2, 0xA4, 0xA1,      //PWCTRL1: Power Control 1 [A4 A1]
            (0xE0), 14, 0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19,     //PVGAMCTRL: Positive Voltage Gamma control        
            (0xE1), 14, 0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19,     //NVGAMCTRL: Negative Voltage Gamma control
            (0x2A), 4, 0x00, 0x00, 0x00, 0xEF,  //X address set
            (0x2B), 4, 0x00, 0x00, 0x01, 0x3F,  //Y address set
            0x29, 0,            //Display On
            TFTLCD_DELAY8, 10,
        };
        static const uint8_t ST7789_regValues_arcain6[] PROGMEM = {
            0x01, 0,            //Soft Reset
            (0x11), 0,          //exit SLEEP mode
            TFTLCD_DELAY8, 100, // if from Sleep
            (0x36), 1, 0x00,    //MADCTL: memory data access control
            (0x3A), 1, 0x05,    //COLMOD: Interface Pixel format  *** I use 65K-colors in 16bit/pixel (5-6-5) format when using 16-bit interface to allow 1-byte per pixel
            (0xB2), 5, 0x0C, 0x0C, 0x00, 0x33, 0x33,    //PORCTRK: Porch setting [08 08 00 22 22] PSEN=0 anyway
            (0xB7), 1, 0x35,    //GCTRL: Gate Control [35]
            (0xBB), 1, 0x35,    //VCOMS: VCOM setting VCOM=??? [20] VCOM=0.9
            (0xC0), 1, 0x2C,    //LCMCTRL: LCM Control [2C]
            (0xC2), 2, 0x01, 0xFF,      //VDVVRHEN: VDV and VRH Command Enable [01 FF]
            (0xC3), 1, 0x13,    //VRHS: VRH Set VAP=???, VAN=-??? [0B]
            (0xC4), 1, 0x20,    //VDVS: VDV Set [20]
            (0xC6), 1, 0x0F,    //FRCTRL2: Frame Rate control in normal mode [0F]
            (0xCA), 1, 0x0F,    //REGSEL2 [0F]
            (0xC8), 1, 0x08,    //REGSEL1 [08]
            (0x55), 1, 0x90,    //WRCACE  [00]
            (0xD0), 2, 0xA4, 0xA1,      //PWCTRL1: Power Control 1 [A4 A1]
            (0xE0), 14, 0xD0, 0x00, 0x06, 0x09, 0x0B, 0x2A, 0x3C, 0x55, 0x4B, 0x08, 0x16, 0x14, 0x19, 0x20,     //PVGAMCTRL: Positive Voltage Gamma control        
            (0xE1), 14, 0xD0, 0x00, 0x06, 0x09, 0x0B, 0x29, 0x36, 0x54, 0x4B, 0x0D, 0x16, 0x14, 0x21, 0x20,     //NVGAMCTRL: Negative Voltage Gamma control
            //        (0x2A), 4, 0x00, 0x00, 0x00, 0xEF,      //X address set
            //        (0x2B), 4, 0x00, 0x00, 0x01, 0x3F,      //Y address set
            0x29, 0,            //Display On
            TFTLCD_DELAY8, 10,
        };
        init_table(ST7789_regValues, sizeof(ST7789_regValues)); //
        break;
#ifdef SUPPORT_8347D
    case 0x4747:       //HX8347-D
        _lcd_capable = REV_SCREEN | MIPI_DCS_REV1 | MV_AXIS | INVERT_SS;
		goto common_8347DGI;
	case 0x7575:       //HX8347-G
	case 0x9595:       //HX8347-I
        _lcd_capable = REV_SCREEN | MIPI_DCS_REV1 | MV_AXIS;
      common_8347DGI:  
		is8347 = 1;
        static const uint8_t HX8347G_2_regValues[] PROGMEM = {
            0xEA, 2, 0x00, 0x20,        //PTBA[15:0]
            0xEC, 2, 0x0C, 0xC4,        //STBA[15:0]
            0xE8, 1, 0x38,      //OPON[7:0]
            0xE9, 1, 0x10,      //OPON1[7:0]
            0xF1, 1, 0x01,      //OTPS1B
            0xF2, 1, 0x10,      //GEN
            //Gamma 2.2 Setting
            0x40, 13, 0x01, 0x00, 0x00, 0x10, 0x0E, 0x24, 0x04, 0x50, 0x02, 0x13, 0x19, 0x19, 0x16,
            0x50, 14, 0x1B, 0x31, 0x2F, 0x3F, 0x3F, 0x3E, 0x2F, 0x7B, 0x09, 0x06, 0x06, 0x0C, 0x1D, 0xCC,
            //Power Voltage Setting
            0x1B, 1, 0x1B,      //VRH=4.65V
            0x1A, 1, 0x01,      //BT (VGH~15V,VGL~-10V,DDVDH~5V)
            0x24, 1, 0x2F,      //VMH(VCOM High voltage ~3.2V)
            0x25, 1, 0x57,      //VML(VCOM Low voltage -1.2V)
            //****VCOM offset**///
            0x23, 1, 0x88,      //for Flicker adjust //can reload from OTP
            //Power on Setting
            0x18, 1, 0x34,      //I/P_RADJ,N/P_RADJ, Normal mode 60Hz
            0x19, 1, 0x01,      //OSC_EN='1', start Osc
            0x01, 1, 0x00,      //DP_STB='0', out deep sleep
            0x1F, 1, 0x88,      // GAS=1, VOMG=00, PON=0, DK=1, XDK=0, DVDH_TRI=0, STB=0
            TFTLCD_DELAY8, 5,
            0x1F, 1, 0x80,      // GAS=1, VOMG=00, PON=0, DK=0, XDK=0, DVDH_TRI=0, STB=0
            TFTLCD_DELAY8, 3,
            0x1F, 1, 0x90,      // GAS=1, VOMG=00, PON=1, DK=0, XDK=0, DVDH_TRI=0, STB=0
            TFTLCD_DELAY8, 5,
            0x1F, 1, 0xD0,      // GAS=1, VOMG=10, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0
            TFTLCD_DELAY8, 5,
            //262k/65k color selection
            0x17, 1, 0x05,      //default 0x06 262k color // 0x05 65k color
            //SET PANEL
            0x36, 1, 0x00,      //SS_P, GS_P,REV_P,BGR_P
            //Display ON Setting
            0x28, 1, 0x38,      //GON=1, DTE=1, D=1000
            TFTLCD_DELAY8, 40,
            0x28, 1, 0x3F,      //GON=1, DTE=1, D=1100

            0x16, 1, 0x18,
            //Set GRAM Area
            0x02, 2, 0x00, 0x00,        //Column Start
            0x04, 2, 0x00, 0xEF,        //Column End
            0x06, 2, 0x00, 0x00,        //Row Start
            0x08, 2, 0x01, 0x3F,        //Row End
        };
        init_table(HX8347G_2_regValues, sizeof(HX8347G_2_regValues));
        break;
#endif
		
#ifdef SUPPORT_8352A
	case 0x5252:       //HX8352-A
        _lcd_capable = MIPI_DCS_REV1 | MV_AXIS;
		is8347 = 1;
        static const uint8_t HX8352A_regValues[] PROGMEM = {
            0x83, 1, 0x02,      //Test Mode: TESTM=1
            0x85, 1, 0x03,      //VDD ctl  : VDC_SEL=3 [05]
            0x8B, 1, 0x01,      //VGS_RES 1: RES_VGS1=1 
            0x8C, 1, 0x93,      //VGS_RES 2: RES_VGS2=1, anon=0x13 [93]
            0x91, 1, 0x01,      //PWM control: SYNC=1
            0x83, 1, 0x00,      //Test Mode: TESTM=0
            //Gamma  Setting
            0x3E, 12, 0xB0, 0x03, 0x10, 0x56, 0x13, 0x46, 0x23, 0x76, 0x00, 0x5E, 0x4F, 0x40,
            //Power Voltage Setting
            0x17, 1, 0x91,      //OSC   1: RADJ=9, OSC_EN=1 [F0]
            0x2B, 1, 0xF9,      //Cycle 1: N_DC=F9 [BE]
            TFTLCD_DELAY8, 10,
            0x1B, 1, 0x14,      //Power 3: BT=1, ??=1, AP=0 [42]  
            0x1A, 1, 0x11,      //Power 2: VC3=1, VC1=1 [05]
            0x1C, 1, 0x06,      //Power 4: VRH=6 [0D]
            0x1F, 1, 0x42,      //VCOM   : VCM=42 [55]
            TFTLCD_DELAY8, 20,
            0x19, 1, 0x0A,      //Power 1: DK=1, VL_TR1=1 [09]
            0x19, 1, 0x1A,      //Power 1: PON=1, DK=1, VL_TR1=1 [09]
            TFTLCD_DELAY8, 40,
            0x19, 1, 0x12,      //Power 1: PON=1, DK=1, STB=1 [09]
            TFTLCD_DELAY8, 40,
            0x1E, 1, 0x27,      //Power 6: VCOMG=1, VDV=7 [10]
            TFTLCD_DELAY8, 100,
            //Display ON Setting
            0x24, 1, 0x60,      //Display 2: PT=1, GON=1 [A0]
            0x3D, 1, 0x40,      //Source 1: N_SAP=40 [C0]
            0x34, 1, 0x38,      //Cycle 10: EQS=0x38 [38]
            0x35, 1, 0x38,      //Cycle 11: EQP=0x38 [38]
            0x24, 1, 0x38,      //Display 2: GON=1 D=2 [A0]
            TFTLCD_DELAY8, 40,
            0x24, 1, 0x3C,      //Display 2: GON=1 D=3 [A0]
            0x16, 1, 0x1C,      //Memaccess: GS=1, BGR=1, SS=1 
            0x01, 1, 0x06,      //Disp Mode: INVON=1, NORON=1 [02]
            0x55, 1, 0x06,      //SM_PANEL=0, SS_PANEL=0, GS_PANEL=1, REV_PANEL=1, BGR_PANEL=0
            //Set GRAM Area
            0x02, 2, 0x00, 0x00,        //Column Start
            0x04, 2, 0x00, 0xEF,        //Column End
            0x06, 2, 0x00, 0x00,        //Row Start
            0x08, 2, 0x01, 0x8F,        //Row End
        };
        init_table(HX8352A_regValues, sizeof(HX8352A_regValues));
        p16 = (int16_t *) & HEIGHT;
        *p16 = 400;
        break;
#endif
#ifdef SUPPORT_8347A
    case 0x8347:
        _lcd_capable = REV_SCREEN | MIPI_DCS_REV1 | MV_AXIS;
        // AN.01 The reference setting of CMO 3.2” Panel
        static const uint8_t HX8347A_CMO32_regValues[] PROGMEM = {
            //  VENDOR Gamma for 3.2"
            (0x46), 12, 0xA4, 0x53, 0x00, 0x44, 0x04, 0x67, 0x33, 0x77, 0x12, 0x4C, 0x46, 0x44,
            //240x320 window setting
            (0x02), 2, 0x00, 0x00,      // Column address start2
            (0x04), 2, 0x00, 0xEF,      // Column address end2
            (0x06), 2, 0x00, 0x00,      // Row address start2
            (0x08), 2, 0x01, 0x3F,      // Row address end2
            // Display Setting
            (0x01), 1, 0x06,    // IDMON=0, INVON=1, NORON=1, PTLON=0
            (0x16), 1, 0x48,    // MY=0, MX=0, MV=0, ML=1, BGR=0, TEON=0
            (0x23), 3, 0x95, 0x95, 0xFF,        // N_DC=1001 0101, PI_DC=1001 0101, I_DC=1111 1111

            (0x27), 4, 0x02, 0x02, 0x02, 0x02,  // N_BP=2, N_FP=2, PI_BP=2, PI_FP=2
            (0x2C), 2, 0x02, 0x02,      // I_BP=2, I_FP=2

            (0x3a), 4, 0x01, 0x01, 0xF0, 0x00,  // N_RTN=0, N_NW=1, P_RTN=0, P_NW=1, I_RTN=15, I_NW=0, DIV=0
            TFTLCD_DELAY8, 5,
            (0x35), 2, 0x38, 0x78,      // EQS=38h, EQP=78h
            (0x3E), 1, 0x38,    // SON=38h
            (0x40), 2, 0x0F, 0xF0,      // GDON=0Fh, GDOFF 
            // Power Supply Setting
            (0x19), 1, 0x49,    // CADJ=0100, CUADJ=100, OSD_EN=1 ,60Hz
            (0x93), 1, 0x0F,    // RADJ=1111, 100%
            TFTLCD_DELAY8, 5,
            (0x20), 1, 0x40,    // BT=0100
            (0x1D), 3, 0x07, 0x00, 0x04,        // VC1=7, VC3=0, VRH=??
            //VCOM SETTING for 3.2"
            (0x44), 2, 0x4D, 0x11,      // VCM=100 1101, VDV=1 0001   
            TFTLCD_DELAY8, 10,
            (0x1C), 1, 0x04,    // AP=100
            TFTLCD_DELAY8, 20,
            (0x1B), 1, 0x18,    // GASENB=0, PON=0, DK=1, XDK=0, VLCD_TRI=0, STB=0
            TFTLCD_DELAY8, 40,
            (0x1B), 1, 0x10,    // GASENB=0, PON=1, DK=0, XDK=0, VLCD_TRI=0, STB=0
            TFTLCD_DELAY8, 40,
            (0x43), 1, 0x80,    //set VCOMG=1
            TFTLCD_DELAY8, 100,
            // Display ON Setting
            (0x90), 1, 0x7F,    // SAP=0111 1111
            (0x26), 1, 0x04,    //GON=0, DTE=0, D=01
            TFTLCD_DELAY8, 40,
            (0x26), 1, 0x24,    //GON=1, DTE=0, D=01
            (0x26), 1, 0x2C,    //GON=1, DTE=0, D=11
            TFTLCD_DELAY8, 40,
            (0x26), 1, 0x3C,    //GON=1, DTE=1, D=11
            // INTERNAL REGISTER SETTING
            (0x57), 1, 0x02,    // TEST_Mode=1: into TEST mode
            (0x55), 1, 0x00,    // VDC_SEL=000, VDDD=1.95V
            (0xFE), 1, 0x5A,    // For ESD protection
            (0x57), 1, 0x00,    // TEST_Mode=0: exit TEST mode
        };
        // AN.01 The reference setting of CMO 2.4” Panel
        static const uint8_t HX8347A_CMO24_regValues[] PROGMEM = {
            //  VENDOR Gamma for 2.4"
            (0x46), 12, 0x94, 0x41, 0x00, 0x33, 0x23, 0x45, 0x44, 0x77, 0x12, 0xCC, 0x46, 0x82,
            //240x320 window setting
            (0x02), 2, 0x00, 0x00,      // Column address start2
            (0x04), 2, 0x00, 0xEF,      // Column address end2
            (0x06), 2, 0x00, 0x00,      // Row address start2
            (0x08), 2, 0x01, 0x3F,      // Row address end2
            // Display Setting
            (0x01), 1, 0x06,    // IDMON=0, INVON=1, NORON=1, PTLON=0
            (0x16), 1, 0x48,    // MY=0, MX=0, MV=0, ML=1, BGR=0, TEON=0
            (0x23), 3, 0x95, 0x95, 0xFF,        // N_DC=1001 0101, PI_DC=1001 0101, I_DC=1111 1111

            (0x27), 4, 0x02, 0x02, 0x02, 0x02,  // N_BP=2, N_FP=2, PI_BP=2, PI_FP=2
            (0x2C), 2, 0x02, 0x02,      // I_BP=2, I_FP=2

            (0x3a), 4, 0x01, 0x01, 0xF0, 0x00,  // N_RTN=0, N_NW=1, P_RTN=0, P_NW=1, I_RTN=15, I_NW=0, DIV=0
            TFTLCD_DELAY8, 5,
            (0x35), 2, 0x38, 0x78,      // EQS=38h, EQP=78h
            (0x3E), 1, 0x38,    // SON=38h
            (0x40), 2, 0x0F, 0xF0,      // GDON=0Fh, GDOFF 
            // Power Supply Setting
            (0x19), 1, 0x49,    // CADJ=0100, CUADJ=100, OSD_EN=1 ,60Hz
            (0x93), 1, 0x0F,    // RADJ=1111, 100%
            TFTLCD_DELAY8, 5,
            (0x20), 1, 0x40,    // BT=0100
            (0x1D), 3, 0x07, 0x00, 0x04,        // VC1=7, VC3=0, VRH=??
            //VCOM SETTING for 2.4"
            (0x44), 2, 0x40, 0x12,      // VCM=100 0000, VDV=1 0001   
            TFTLCD_DELAY8, 10,
            (0x1C), 1, 0x04,    // AP=100
            TFTLCD_DELAY8, 20,
            (0x1B), 1, 0x18,    // GASENB=0, PON=0, DK=1, XDK=0, VLCD_TRI=0, STB=0
            TFTLCD_DELAY8, 40,
            (0x1B), 1, 0x10,    // GASENB=0, PON=1, DK=0, XDK=0, VLCD_TRI=0, STB=0
            TFTLCD_DELAY8, 40,
            (0x43), 1, 0x80,    //set VCOMG=1
            TFTLCD_DELAY8, 100,
            // Display ON Setting
            (0x90), 1, 0x7F,    // SAP=0111 1111
            (0x26), 1, 0x04,    //GON=0, DTE=0, D=01
            TFTLCD_DELAY8, 40,
            (0x26), 1, 0x24,    //GON=1, DTE=0, D=01
            (0x26), 1, 0x2C,    //GON=1, DTE=0, D=11
            TFTLCD_DELAY8, 40,
            (0x26), 1, 0x3C,    //GON=1, DTE=1, D=11
            // INTERNAL REGISTER SETTING
            (0x57), 1, 0x02,    // TEST_Mode=1: into TEST mode
            (0x55), 1, 0x00,    // VDC_SEL=000, VDDD=1.95V
            (0xFE), 1, 0x5A,    // For ESD protection
            (0x57), 1, 0x00,    // TEST_Mode=0: exit TEST mode
        };
        static const uint8_t HX8347A_ITDB02_regValues[] PROGMEM = {
            //  VENDOR Gamma ITDB02 same as CMO32.   Delays are shorter than AN01
            (0x46), 12, 0xA4, 0x53, 0x00, 0x44, 0x04, 0x67, 0x33, 0x77, 0x12, 0x4C, 0x46, 0x44,
            //240x320 window setting
            (0x02), 2, 0x00, 0x00,      // Column address start2
            (0x04), 2, 0x00, 0xEF,      // Column address end2
            (0x06), 2, 0x00, 0x00,      // Row address start2
            (0x08), 2, 0x01, 0x3F,      // Row address end2
            // Display Setting
            (0x01), 1, 0x06,    // IDMON=0, INVON=1, NORON=1, PTLON=0
            (0x16), 1, 0xC8,    // MY=0, MX=0, MV=0, ML=1, BGR=0, TEON=0 .itead
            (0x23), 3, 0x95, 0x95, 0xFF,        // N_DC=1001 0101, PI_DC=1001 0101, I_DC=1111 1111

            (0x27), 4, 0x02, 0x02, 0x02, 0x02,  // N_BP=2, N_FP=2, PI_BP=2, PI_FP=2
            (0x2C), 2, 0x02, 0x02,      // I_BP=2, I_FP=2

            (0x3a), 4, 0x01, 0x00, 0xF0, 0x00,  // N_RTN=0, N_NW=1, P_RTN=0, ?? P_NW=1, I_RTN=15, I_NW=0, DIV=0 .itead
            TFTLCD_DELAY8, 5,
            (0x35), 2, 0x38, 0x78,      // EQS=38h, EQP=78h
            (0x3E), 1, 0x38,    // SON=38h
            (0x40), 2, 0x0F, 0xF0,      // GDON=0Fh, GDOFF 
            // Power Supply Setting 
            (0x19), 1, 0x49,    // CADJ=0100, CUADJ=100, OSD_EN=1 ,60Hz
            (0x93), 1, 0x0F,    // RADJ=1111, 100%
            TFTLCD_DELAY8, 5,
            (0x20), 1, 0x40,    // BT=0100
            (0x1D), 3, 0x07, 0x00, 0x04,        // VC1=7, VC3=0, VRH=??
            //VCOM SETTING for ITDB02
            (0x44), 2, 0x4D, 0x0E,      // VCM=101 0000  4D, VDV=1 0001 .itead
            TFTLCD_DELAY8, 5,
            (0x1C), 1, 0x04,    // AP=100
            TFTLCD_DELAY8, 5,
            (0x1B), 1, 0x18,    // GASENB=0, PON=0, DK=1, XDK=0, VLCD_TRI=0, STB=0
            TFTLCD_DELAY8, 5,
            (0x1B), 1, 0x10,    // GASENB=0, PON=1, DK=0, XDK=0, VLCD_TRI=0, STB=0
            TFTLCD_DELAY8, 5,
            (0x43), 1, 0x80,    //set VCOMG=1
            TFTLCD_DELAY8, 5,
            // Display ON Setting
            (0x90), 1, 0x7F,    // SAP=0111 1111
            (0x26), 1, 0x04,    //GON=0, DTE=0, D=01
            TFTLCD_DELAY8, 5,
            (0x26), 1, 0x24,    //GON=1, DTE=0, D=01
            (0x26), 1, 0x2C,    //GON=1, DTE=0, D=11
            TFTLCD_DELAY8, 5,
            (0x26), 1, 0x3C,    //GON=1, DTE=1, D=11
            // INTERNAL REGISTER SETTING for ITDB02
            (0x57), 1, 0x02,    // TEST_Mode=1: into TEST mode
            (0x95), 1, 0x01,    // SET DISPLAY CLOCK AND PUMPING CLOCK TO SYNCHRONIZE .itead
            (0x57), 1, 0x00,    // TEST_Mode=0: exit TEST mode
        };
        static const uint8_t HX8347A_NHD_regValues[] PROGMEM = {
            //Gamma Setting NHD
            (0x46), 12, 0x94, 0x41, 0x00, 0x33, 0x23, 0x45, 0x44, 0x77, 0x12, 0xCC, 0x46, 0x82,
            (0x02), 2, 0x00, 0x00,      //COLSTARTH
            (0x04), 2, 0x00, 0xEF,      //COLENDH
            (0x06), 2, 0x00, 0x00,      //ROWSTARTH
            (0x08), 2, 0x01, 0x3F,      //ROWENDH
            (0x01), 1, 0x06,    //Display Mode [06]
            (0x16), 1, 0xC8,    //MADCTL [00] MY=1, MX=1, BGR=1
//            (0x70), 1, 0x05,    //Panel [06] 16-bit
            (0x23), 3, 0x95, 0x95, 0xFF,        //Cycle Control 1-3 [95 95 FF]
            (0x27), 4, 0x02, 0x02, 0x02, 0x02,  //Display Control 2-5 [02 02 02 02]
            (0x2C), 2, 0x02, 0x02,      //Display Control 6-7 [02 02]
            (0x3A), 4, 0x01, 0x01, 0xF0, 0x00,  //Cycle Control 1-4 [01 01 F0 00]
            TFTLCD_DELAY8, 80,
            (0x35), 2, 0x38, 0x78,      //Display Control 9-10 [09 09] EQS=56, EQP=120
            (0x3E), 1, 0x38,    //Cycle Control 5 [38]  
            (0x40), 1, 0x0F,    //Cycle Control 6 [03]  GDON=15
            (0x41), 1, 0xF0,    //Cycle Control 14 [F8] GDOF=248

            (0x19), 1, 0x2D,    //OSC Control 1 [86] CADJ=2, CUADJ=6, OSCEN=1
            (0x93), 1, 0x06,    //SAP Idle mode [00] ???  .nhd
            TFTLCD_DELAY8, 80,
            (0x20), 1, 0x40,    //Power Control 6 [40]
            (0x1D), 3, 0x07, 0x00, 0x04,        //Power Control 3-5 [04 00 06] VC=7
            (0x44), 2, 0x3C, 0x12,      //VCOM Control 2-3 [5A 11] VCM=60, VDV=18
            TFTLCD_DELAY8, 80,
            (0x1C), 1, 0x04,    //Power Control 2 [04]
            TFTLCD_DELAY8, 80,
            (0x43), 1, 0x80,    //VCOM Control 1 [80]
            TFTLCD_DELAY8, 80,
            (0x1B), 1, 0x08,    //Power Control 1 [00] DK=1
            TFTLCD_DELAY8, 80,
            (0x1B), 1, 0x10,    //Power Control 1 [00] PON=1
            TFTLCD_DELAY8, 80,
            (0x90), 1, 0x7F,    //Display Control 8 [0A]
            (0x26), 1, 0x04,    //Display Control 1 [A0] D=1
            TFTLCD_DELAY8, 80,
            (0x26), 1, 0x24,    //Display Control 1 [A0] GON=1, D=1
            (0x26), 1, 0x2C,    //Display Control 1 [A0] GON=1, D=3 
            TFTLCD_DELAY8, 80,
            (0x26), 1, 0x3C,    //Display Control 1 [A0] GON=1, DTE=1, D=3
            (0x57), 1, 0x02,    //?
            (0x55), 1, 0x00,    //?
            (0x57), 1, 0x00,    //? 
        };
        // Atmel ASF code uses VCOM2-3: 0x38, 0x12. 50ms delays and no TEST mode changes.
        init_table(HX8347A_NHD_regValues, sizeof(HX8347A_NHD_regValues));
        //        init_table(HX8347A_CMO32_regValues, sizeof(HX8347A_CMO32_regValues));
        //        init_table(HX8347A_CMO24_regValues, sizeof(HX8347A_CMO24_regValues));
        //        init_table(HX8347A_ITDB02_regValues, sizeof(HX8347A_ITDB02_regValues));
        //        init_table(HX8347G_2_regValues, sizeof(HX8347G_2_regValues));
        break;
#endif

    case 0x8357:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | REV_SCREEN | READ_24BITS;
        static const uint8_t HX8357D_regValues[] PROGMEM = {
	    	0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 50,
            0x28, 0,            //Display Off
            0x3A, 1, 0x55,      //Interlace Pixel
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 50,
            0x29, 0,            //Display On
        };
        init_table(HX8357D_regValues, sizeof(HX8357D_regValues));
        p16 = (int16_t *) & HEIGHT;
        *p16 = 480;
        p16 = (int16_t *) & WIDTH;
        *p16 = 320;
        break;

    case 0x5408:
        _lcd_capable = 0 | REV_SCREEN | READ_BGR | INVERT_GS;
        goto common_9320;
    case 0x9320:
        _lcd_capable = 0 | REV_SCREEN | READ_BGR;
      common_9320:
        static const uint16_t ILI9320_regValues[] PROGMEM = {
            0x00e5, 0x8000,
            0x0000, 0x0001,
            0x0001, 0x100,
            0x0002, 0x0700,
            0x0003, 0x1030,
            0x0004, 0x0000,
            0x0008, 0x0202,
            0x0009, 0x0000,
            0x000A, 0x0000,
            0x000C, 0x0000,
            0x000D, 0x0000,
            0x000F, 0x0000,
            //-----Power On sequence-----------------------
            0x0010, 0x0000,
            0x0011, 0x0007,
            0x0012, 0x0000,
            0x0013, 0x0000,
            TFTLCD_DELAY, 50,
            0x0010, 0x17B0,
            0x0011, 0x0007,
            TFTLCD_DELAY, 10,
            0x0012, 0x013A,
            TFTLCD_DELAY, 10,
            0x0013, 0x1A00,
            0x0029, 0x000c,
            TFTLCD_DELAY, 10,
            //-----Gamma control-----------------------
            0x0030, 0x0000,
            0x0031, 0x0505,
            0x0032, 0x0004,
            0x0035, 0x0006,
            0x0036, 0x0707,
            0x0037, 0x0105,
            0x0038, 0x0002,
            0x0039, 0x0707,
            0x003C, 0x0704,
            0x003D, 0x0807,
            //-----Set RAM area-----------------------
            0x0050, 0x0000,
            0x0051, 0x00EF,
            0x0052, 0x0000,
            0x0053, 0x013F,
            0x0060, 0xA700,     //GS=1
            0x0061, 0x0001,
            0x006A, 0x0000,
            0x0021, 0x0000,
            0x0020, 0x0000,
            //-----Partial Display Control------------
            0x0080, 0x0000,
            0x0081, 0x0000,
            0x0082, 0x0000,
            0x0083, 0x0000,
            0x0084, 0x0000,
            0x0085, 0x0000,
            //-----Panel Control----------------------
            0x0090, 0x0010,
            0x0092, 0x0000,
            0x0093, 0x0003,
            0x0095, 0x0110,
            0x0097, 0x0000,
            0x0098, 0x0000,
            //-----Display on-----------------------
            0x0007, 0x0173,
            TFTLCD_DELAY, 50,
        };
        init_table16(ILI9320_regValues, sizeof(ILI9320_regValues));
        break;
    case 0x6809:
        _lcd_capable = 0 | REV_SCREEN | INVERT_GS | AUTO_READINC;
        goto common_93x5;
    case 0x9328:
	case 0x9325:
        _lcd_capable = 0 | REV_SCREEN | INVERT_GS;
        goto common_93x5;
    case 0x9331:
	case 0x9335:
        _lcd_capable = 0 | REV_SCREEN;
      common_93x5:
        static const uint16_t ILI9325_regValues[] PROGMEM = {
            0x00E5, 0x78F0,     // set SRAM internal timing
            0x0001, 0x0100,     // set Driver Output Control
            0x0002, 0x0200,     // set 1 line inversion
            0x0003, 0x1030,     // set GRAM write direction and BGR=1.
            0x0004, 0x0000,     // Resize register
            0x0005, 0x0000,     // .kbv 16bits Data Format Selection
            0x0008, 0x0207,     // set the back porch and front porch
            0x0009, 0x0000,     // set non-display area refresh cycle ISC[3:0]
            0x000A, 0x0000,     // FMARK function
            0x000C, 0x0000,     // RGB interface setting
            0x000D, 0x0000,     // Frame marker Position
            0x000F, 0x0000,     // RGB interface polarity
            // ----------- Power On sequence ----------- //
            0x0010, 0x0000,     // SAP, BT[3:0], AP, DSTB, SLP, STB
            0x0011, 0x0007,     // DC1[2:0], DC0[2:0], VC[2:0]
            0x0012, 0x0000,     // VREG1OUT voltage
            0x0013, 0x0000,     // VDV[4:0] for VCOM amplitude
            0x0007, 0x0001,
            TFTLCD_DELAY, 200,  // Dis-charge capacitor power voltage
            0x0010, 0x1690,     // SAP, BT[3:0], AP, DSTB, SLP, STB
            0x0011, 0x0227,     // Set DC1[2:0], DC0[2:0], VC[2:0]
            TFTLCD_DELAY, 50,   // wait_ms 50ms
            0x0012, 0x000D,     // 0012
            TFTLCD_DELAY, 50,   // wait_ms 50ms
            0x0013, 0x1200,     // VDV[4:0] for VCOM amplitude
            0x0029, 0x000A,     // 04  VCM[5:0] for VCOMH
            0x002B, 0x000D,     // Set Frame Rate
            TFTLCD_DELAY, 50,   // wait_ms 50ms
            0x0020, 0x0000,     // GRAM horizontal Address
            0x0021, 0x0000,     // GRAM Vertical Address
            // ----------- Adjust the Gamma Curve ----------//

            0x0030, 0x0000,
            0x0031, 0x0404,
            0x0032, 0x0003,
            0x0035, 0x0405,
            0x0036, 0x0808,
            0x0037, 0x0407,
            0x0038, 0x0303,
            0x0039, 0x0707,
            0x003C, 0x0504,
            0x003D, 0x0808,

            //------------------ Set GRAM area ---------------//
            0x0050, 0x0000,     // Horizontal GRAM Start Address
            0x0051, 0x00EF,     // Horizontal GRAM End Address
            0x0052, 0x0000,     // Vertical GRAM Start Address
            0x0053, 0x013F,     // Vertical GRAM Start Address
            0x0060, 0x2700,     // Gate Scan Line GS=0 [0xA700] 
            0x0061, 0x0001,     // NDL,VLE, REV .kbv
            0x006A, 0x0000,     // set scrolling line
            //-------------- Partial Display Control ---------//
            0x0080, 0x0000,
            0x0081, 0x0000,
            0x0082, 0x0000,
            0x0083, 0x0000,
            0x0084, 0x0000,
            0x0085, 0x0000,
            //-------------- Panel Control -------------------//
            0x0090, 0x0010,
            0x0092, 0x0000,
            0x0007, 0x0133,     // 262K color and display ON
        };
        init_table16(ILI9325_regValues, sizeof(ILI9325_regValues));
        break;
	case 0x5420:
    case 0x9326:
        _lcd_capable = REV_SCREEN | READ_BGR;
        static const uint16_t ILI9326_CPT28_regValues[] PROGMEM = {
//************* Start Initial Sequence **********//
         0x0702, 0x3008,     //  Set internal timing, don’t change this value
         0x0705, 0x0036,     //  Set internal timing, don’t change this value
         0x070B, 0x1213,     //  Set internal timing, don’t change this value
         0x0001, 0x0100,     //  set SS and SM bit
         0x0002, 0x0100,     //  set 1 line inversion
         0x0003, 0x1030,     //  set GRAM write direction and BGR=1.
         0x0008, 0x0202,     //  set the back porch and front porch
         0x0009, 0x0000,     //  set non-display area refresh cycle ISC[3:0]
         0x000C, 0x0000,     //  RGB interface setting
         0x000F, 0x0000,     //  RGB interface polarity
//*************Power On sequence ****************//
         0x0100, 0x0000,     //  SAP, BT[3:0], AP, DSTB, SLP, STB
         0x0102, 0x0000,     //  VREG1OUT voltage
         0x0103, 0x0000,   // VDV[4:0] for VCOM amplitude
        TFTLCD_DELAY, 200,   // Dis-charge capacitor power voltage
         0x0100, 0x1190,   // SAP, BT[3:0], AP, DSTB, SLP, STB
         0x0101, 0x0227,   // DC1[2:0], DC0[2:0], VC[2:0]
        TFTLCD_DELAY, 50,   // Delay 50ms
         0x0102, 0x01BD,   // VREG1OUT voltage
        TFTLCD_DELAY, 50,   // Delay 50ms
         0x0103, 0x2D00,   // VDV[4:0] for VCOM amplitude
         0x0281, 0x000E,   // VCM[5:0] for VCOMH
        TFTLCD_DELAY, 50,   //
         0x0200, 0x0000,   // GRAM horizontal Address
         0x0201, 0x0000,   // GRAM Vertical Address
// ----------- Adjust the Gamma Curve ----------//
         0x0300, 0x0000,   //
         0x0301, 0x0707,   //
         0x0302, 0x0606,   //
         0x0305, 0x0000,   //
         0x0306, 0x0D00,   //
         0x0307, 0x0706,   //
         0x0308, 0x0005,   //
         0x0309, 0x0007,   //
         0x030C, 0x0000,   //
         0x030D, 0x000A,   //
//------------------ Set GRAM area ---------------//
         0x0210, 0x0000,     //  Horizontal GRAM Start Address
         0x0211, 0x00EF,     //  Horizontal GRAM End Address
         0x0212, 0x0000,     //  Vertical GRAM Start Address
         0x0213, 0x01AF,     //  Vertical GRAM Start Address
         0x0400, 0x3100,     //  Gate Scan Line 400 lines
         0x0401, 0x0001,     //  NDL,VLE, REV
         0x0404, 0x0000,     //  set scrolling line
//-------------- Partial Display Control ---------//
         0x0500, 0x0000,     // Partial Image 1 Display Position
         0x0501, 0x0000,     // Partial Image 1 RAM Start/End Address
         0x0502, 0x0000,     // Partial Image 1 RAM Start/End Address
         0x0503, 0x0000,     // Partial Image 2 Display Position
         0x0504, 0x0000,     // Partial Image 2 RAM Start/End Address
         0x0505, 0x0000,     // Partial Image 2 RAM Start/End Address
//-------------- Panel Control -------------------//
         0x0010, 0x0010,     // DIVI[1:0];RTNI[4:0]
         0x0011, 0x0600,     // NOWI[2:0];SDTI[2:0]
         0x0020, 0x0002,     // DIVE[1:0];RTNE[5:0]
         0x0007, 0x0173,     //  262K color and display ON
		 };
        init_table16(ILI9326_CPT28_regValues, sizeof(ILI9326_CPT28_regValues));
        p16 = (int16_t *) & HEIGHT;
        *p16 = 400;
        p16 = (int16_t *) & WIDTH;
        *p16 = 240;
        break;
    case 0x9327:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS;
        static const uint8_t ILI9327_regValues[] PROGMEM = {
            0x01, 0,            //Soft Reset
            0x28, 0,            //Display Off
            //            0xE0, 1, 0x20,      //NV Memory Write [00]
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 100,
            0xB0, 1, 0x00,      //Disable Protect for cmds B1-DF, E0-EF, F0-FF 
            //            0xD1, 3, 0x00, 0x71, 0x19,  //VCOM control [00 40 0F]
            //            0xD0, 3, 0x07, 0x01, 0x08,  //Power Setting [07 04 8C]
            0xC1, 4, 0x10, 0x10, 0x02, 0x02,    //Display Timing [10 10 02 02]
            0xC0, 6, 0x00, 0x35, 0x00, 0x00, 0x01, 0x02,        //Panel Drive [00 35 00 00 01 02 REV=0,GS=0,SS=0
            0xC5, 1, 0x04,      //Frame Rate [04]
            0xD2, 2, 0x01, 0x04,        //Power Setting [01 44]
            //            0xC8, 15, 0x04, 0x67, 0x35, 0x04, 0x08, 0x06, 0x24, 0x01, 0x37, 0x40, 0x03, 0x10, 0x08, 0x80, 0x00,
            //            0xC8, 15, 0x00, 0x77, 0x77, 0x04, 0x04, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
            0xCA, 1, 0x00,      //DGC LUT ???
            0xEA, 1, 0x80,      //3-Gamma Function Enable
            //                     0xB0, 1, 0x03,      //Enable Protect
            0x36, 1, 0x48,      // Memory Access
            0x3A, 1, 0x55,      //Pixel read=565, write=565
            0x2A, 4, 0x00, 0x00, 0x00, 0xEF,    // wid: 0, 239
            0x2B, 4, 0x00, 0x00, 0x01, 0x8F,    // ht: 0, 399
            0x30, 4, 0x00, 0x00, 0x01, 0x8F,    // Partial Area: 0, 399
            0x29, 0,            //Display On
        };
        init_table(ILI9327_regValues, sizeof(ILI9327_regValues));
        p16 = (int16_t *) & HEIGHT;
        *p16 = 400;
        p16 = (int16_t *) & WIDTH;
        *p16 = 240;
        break;
    case 0x1602:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | INVERT_GS | READ_24BITS; //thanks Dumper
		goto common_9329;
    case 0xAC11:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | READ_24BITS | REV_SCREEN; //thanks viliam
		goto common_9329;
    case 0x9302:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS;
		goto common_9329;
    case 0x9338:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | READ_24BITS;
		goto common_9329;
    case 0x9329:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | INVERT_SS | REV_SCREEN;
	  common_9329:
        static const uint8_t ILI9329_regValues[] PROGMEM = {
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 50,  // .kbv will power up with ONLY reset, sleep out, display on
            0x28, 0,            //Display Off
//            0xF6, 3, 0x01, 0x01, 0x00,  //Interface Control needs EXTC=1 MX_EOR=1, TM=0, RIM=0
//            0xB6, 3, 0x0A, 0x82, 0x27,  //Display Function [0A 82 27]
//            0xB7, 1, 0x06,      //Entry Mode Set [06]
			0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 150,
            0x29, 0,            //Display On
            0x3A, 1, 0x55,      //Pixel read=565, write=565
        };
        init_table(ILI9329_regValues, sizeof(ILI9329_regValues));
        break;
    case 0x9340:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | READ_24BITS | REV_SCREEN;
        static const uint8_t ILI9340_regValues_2_4[] PROGMEM = {        // BOE 2.4"
            0x01, 0,            // software reset
            TFTLCD_DELAY8, 50,  // .kbv will power up with ONLY reset, sleep out, display on
            0x28, 0,            //Display Off
            0xF6, 3, 0x01, 0x01, 0x00,  //Interface Control needs EXTC=1 MV_EOR=0, TM=0, RIM=0
            0xCF, 3, 0x00, 0x81, 0x30,  //Power Control B [00 81 30]
            0xED, 4, 0x64, 0x03, 0x12, 0x81,    //Power On Seq [55 01 23 01]
            0xE8, 3, 0x85, 0x10, 0x78,  //Driver Timing A [04 11 7A]
            0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,      //Power Control A [39 2C 00 34 02]
            0xF7, 1, 0x20,      //Pump Ratio [10]
            0xEA, 2, 0x00, 0x00,        //Driver Timing B [66 00]
            0xB0, 1, 0x00,      //RGB Signal [00]
            0xB1, 2, 0x00, 0x1B,        //Frame Control [00 1B]
            //            0xB6, 2, 0x0A, 0xA2, 0x27, //Display Function [0A 82 27 XX]    .kbv SS=1
            0xB4, 1, 0x00,      //Inversion Control [02] .kbv NLA=1, NLB=1, NLC=1
            0xC0, 1, 0x21,      //Power Control 1 [26]
            0xC1, 1, 0x11,      //Power Control 2 [00]
            0xC5, 2, 0x3F, 0x3C,        //VCOM 1 [31 3C]
            0xC7, 1, 0xB5,      //VCOM 2 [C0]
            0x36, 1, 0x48,      //Memory Access [00]
            0xF2, 1, 0x00,      //Enable 3G [02]
            0x26, 1, 0x01,      //Gamma Set [01]
            0xE0, 15, 0x0f, 0x26, 0x24, 0x0b, 0x0e, 0x09, 0x54, 0xa8, 0x46, 0x0c, 0x17, 0x09, 0x0f, 0x07, 0x00,
            0xE1, 15, 0x00, 0x19, 0x1b, 0x04, 0x10, 0x07, 0x2a, 0x47, 0x39, 0x03, 0x06, 0x06, 0x30, 0x38, 0x0f,
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 150,
            0x29, 0,            //Display On
            0x3A, 1, 0x55,      //Pixel Format [66]
        };
        static const uint8_t ILI9340_regValues_ada[] PROGMEM = {        // Adafruit_TFTLCD only works with EXTC=0
            0x01, 0,            // software reset
            TFTLCD_DELAY8, 50,
            0x28, 0,            //Display Off
            //                     0xF6, 3, 0x00, 0x01, 0x00,  //Interface Control needs EXTC=1 TM=0, RIM=0
            //            0xF6, 3, 0x01, 0x01, 0x03,  //Interface Control needs EXTC=1 RM=1, RIM=1
            0xF6, 3, 0x09, 0x01, 0x03,  //Interface Control needs EXTC=1 RM=0, RIM=1
            0xB0, 1, 0x40,      //RGB Signal [40] RCM=2
            0xB4, 1, 0x00,      //Inversion Control [02] .kbv NLA=1, NLB=1, NLC=1
            0xC0, 1, 0x23,      //Power Control 1 [26]
            0xC1, 1, 0x10,      //Power Control 2 [00]
            0xC5, 2, 0x2B, 0x2B,        //VCOM 1 [31 3C]
            0xC7, 1, 0xC0,      //VCOM 2 [C0]
            0x36, 1, 0x88,      //Memory Access [00]
            0xB1, 2, 0x00, 0x1B,        //Frame Control [00 1B]
            0xB7, 1, 0x07,      //Entry Mode [00]
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 150,
            0x29, 0,            //Display On
            0x3A, 1, 0x55,      //Pixel Format [66]
        };
#if !defined(USE_SERIAL)
        if (readReg32(0xD3) == 0x0000) {        //weird DealExtreme EXTC=0 shield
            init_table(ILI9340_regValues_ada, sizeof(ILI9340_regValues_ada));
            _lcd_capable |= REV_SCREEN | READ_BGR;
        } else
#endif
        {
            init_table(ILI9340_regValues_2_4, sizeof(ILI9340_regValues_2_4));   //
        }
        break;
    case 0x9341:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | READ_24BITS;
        static const uint8_t ILI9341_regValues_2_4[] PROGMEM = {        // BOE 2.4"
            0x01, 0,            // software reset
            TFTLCD_DELAY8, 50,  // .kbv will power up with ONLY reset, sleep out, display on
            0x28, 0,            //Display Off
            0xF6, 3, 0x01, 0x01, 0x00,  //Interface Control needs EXTC=1 MV_EOR=0, TM=0, RIM=0
            0xCF, 3, 0x00, 0x81, 0x30,  //Power Control B [00 81 30]
            0xED, 4, 0x64, 0x03, 0x12, 0x81,    //Power On Seq [55 01 23 01]
            0xE8, 3, 0x85, 0x10, 0x78,  //Driver Timing A [04 11 7A]
            0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,      //Power Control A [39 2C 00 34 02]
            0xF7, 1, 0x20,      //Pump Ratio [10]
            0xEA, 2, 0x00, 0x00,        //Driver Timing B [66 00]
            0xB0, 1, 0x00,      //RGB Signal [00] 
            0xB1, 2, 0x00, 0x1B,        //Frame Control [00 1B]
            //            0xB6, 2, 0x0A, 0xA2, 0x27, //Display Function [0A 82 27 XX]    .kbv SS=1  
            0xB4, 1, 0x00,      //Inversion Control [02] .kbv NLA=1, NLB=1, NLC=1
            0xC0, 1, 0x21,      //Power Control 1 [26]
            0xC1, 1, 0x11,      //Power Control 2 [00]
            0xC5, 2, 0x3F, 0x3C,        //VCOM 1 [31 3C]
            0xC7, 1, 0xB5,      //VCOM 2 [C0]
            0x36, 1, 0x48,      //Memory Access [00]
            0xF2, 1, 0x00,      //Enable 3G [02]
            0x26, 1, 0x01,      //Gamma Set [01]
            0xE0, 15, 0x0f, 0x26, 0x24, 0x0b, 0x0e, 0x09, 0x54, 0xa8, 0x46, 0x0c, 0x17, 0x09, 0x0f, 0x07, 0x00,
            0xE1, 15, 0x00, 0x19, 0x1b, 0x04, 0x10, 0x07, 0x2a, 0x47, 0x39, 0x03, 0x06, 0x06, 0x30, 0x38, 0x0f,
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 150,
            0x29, 0,            //Display On
            0x3A, 1, 0x55,      //Pixel Format [66]
        };
        static const uint8_t ILI9341_regValues_ada[] PROGMEM = {        // Adafruit_TFTLCD only works with EXTC=0
            0x01, 0,            // software reset
            TFTLCD_DELAY8, 50,
            0x28, 0,            //Display Off
            //                     0xF6, 3, 0x00, 0x01, 0x00,  //Interface Control needs EXTC=1 TM=0, RIM=0
            //            0xF6, 3, 0x01, 0x01, 0x03,  //Interface Control needs EXTC=1 RM=1, RIM=1
            0xF6, 3, 0x09, 0x01, 0x03,  //Interface Control needs EXTC=1 RM=0, RIM=1
            0xB0, 1, 0x40,      //RGB Signal [40] RCM=2
            0xB4, 1, 0x00,      //Inversion Control [02] .kbv NLA=1, NLB=1, NLC=1
            0xC0, 1, 0x23,      //Power Control 1 [26]
            0xC1, 1, 0x10,      //Power Control 2 [00]
            0xC5, 2, 0x2B, 0x2B,        //VCOM 1 [31 3C]
            0xC7, 1, 0xC0,      //VCOM 2 [C0]
            0x36, 1, 0x88,      //Memory Access [00]
            0xB1, 2, 0x00, 0x1B,        //Frame Control [00 1B]
            0xB7, 1, 0x07,      //Entry Mode [00]
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 150,
            0x29, 0,            //Display On
            0x3A, 1, 0x55,      //Pixel Format [66]
        };
#if !defined(USE_SERIAL)
        if (readReg32(0xD3) == 0x0000) {        //weird DealExtreme EXTC=0 shield
            init_table(ILI9341_regValues_ada, sizeof(ILI9341_regValues_ada));
            _lcd_capable |= REV_SCREEN | READ_BGR;
        } else
#endif
        {
            init_table(ILI9341_regValues_2_4, sizeof(ILI9341_regValues_2_4));   //
        }
        break;
    case 0x1581:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | READ_BGR | READ_24BITS; //thanks zdravke
		goto common_9481;
    case 0x9481:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | READ_BGR;
	  common_9481:
        static const uint8_t ILI9481_regValues[] PROGMEM = {    // Atmel MaxTouch
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 125,
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 20,
            0xB0, 1, 0x00,              // unlocks E0, F0
            0xB3, 4, 0x02, 0x00, 0x00, 0x00, //Frame Memory, interface [02 00 00 00]
			0xB4, 1, 0x00,              // Frame mode [00]
			0xD0, 3, 0x07, 0x42, 0x18,  // Set Power [00 43 18] x1.00, x6, x3
            0xD1, 3, 0x00, 0x07, 0x10,  // Set VCOM  [00 00 00] x0.72, x1.02
            0xD2, 2, 0x01, 0x02,        // Set Power for Normal Mode [01 22]
            0xD3, 2, 0x01, 0x02,        // Set Power for Partial Mode [01 22]
            0xD4, 2, 0x01, 0x02,        // Set Power for Idle Mode [01 22]
            0xC0, 5, 0x10, 0x3B, 0x00, 0x02, 0x11,      //Set Panel Driving [10 3B 00 02 11]
            0xC1, 3, 0x10, 0x10, 0x88,  // Display Timing Normal [10 10 88]
			0xC5, 1, 0x03,      //Frame Rate [03]
			0xC6, 1, 0x02,      //Interface Control [02]
            0xC8, 12, 0x00, 0x32, 0x36, 0x45, 0x06, 0x16, 0x37, 0x75, 0x77, 0x54, 0x0C, 0x00,
			0xCC, 1, 0x00,      //Panel Control [00]
//            0x36, 1, 0x0A,      //Memory Access [00]

            0x3A, 1, 0x55,      //Interlace Pixel Format [XX]
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 120,
            0x29, 0,            //Display On
            TFTLCD_DELAY8, 25,
        };
        static const uint8_t ILI9481_CPT29_regValues[] PROGMEM = {    // 320x430
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 125,
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 20,
            0xB0, 1, 0x00,
            0xD0, 3, 0x07, 0x42, 0x1C,  // Set Power [00 43 18]
            0xD1, 3, 0x00, 0x02, 0x0F,  // Set VCOM  [00 00 00] x0.695, x1.00
            0xD2, 2, 0x01, 0x11,        // Set Power for Normal Mode [01 22]
            0xC0, 5, 0x10, 0x35, 0x00, 0x02, 0x11,      //Set Panel Driving [10 3B 00 02 11]
            0xC5, 1, 0x03,      //Frame Rate [03]
            0xC8, 12, 0x00, 0x30, 0x36, 0x45, 0x04, 0x16, 0x37, 0x75, 0x77, 0x54, 0x0F, 0x00,
            0xE4, 1, 0xA0,
            0xF0, 1, 0x01,
            0xF3, 2, 0x02, 0x1A,			
            0x36, 1, 0x0A,      //Memory Access [00]
            0x2A, 4, 0x00, 0x00, 0x01, 0x3F,
            0x2B, 4, 0x00, 0x00, 0x01, 0xAD,		

            0x3A, 1, 0x55,      //Interlace Pixel Format [XX]
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 120,
            0x29, 0,            //Display On
            TFTLCD_DELAY8, 25,
        };
        static const uint8_t ILI9481_PVI35_regValues[] PROGMEM = {    // 320x480
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 125,
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 20,
            0xB0, 1, 0x00,
            0xD0, 3, 0x07, 0x41, 0x1D,  // Set Power [00 43 18]
            0xD1, 3, 0x00, 0x2B, 0x1F,  // Set VCOM  [00 00 00] x0.900, x1.32
            0xD2, 2, 0x01, 0x11,        // Set Power for Normal Mode [01 22]
            0xC0, 5, 0x10, 0x3B, 0x00, 0x02, 0x11,      //Set Panel Driving [10 3B 00 02 11]
            0xC5, 1, 0x03,      //Frame Rate [03]
            0xC8, 12, 0x00, 0x14, 0x33, 0x10, 0x00, 0x16, 0x44, 0x36, 0x77, 0x00, 0x0F, 0x00,
            0xE4, 1, 0xA0,
            0xF0, 1, 0x01,
            0xF3, 2, 0x40, 0x0A,			
            0x36, 1, 0x0A,      //Memory Access [00]

            0x3A, 1, 0x55,      //Interlace Pixel Format [XX]
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 120,
            0x29, 0,            //Display On
            TFTLCD_DELAY8, 25,
        };
        static const uint8_t ILI9481_AUO317_regValues[] PROGMEM = {    // 320x480
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 125,
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 20,
            0xB0, 1, 0x00,
            0xD0, 3, 0x07, 0x40, 0x1D,  // Set Power [00 43 18]
            0xD1, 3, 0x00, 0x18, 0x13,  // Set VCOM  [00 00 00] x0.805, x1.08
            0xD2, 2, 0x01, 0x11,        // Set Power for Normal Mode [01 22]
            0xC0, 5, 0x10, 0x3B, 0x00, 0x02, 0x11,      //Set Panel Driving [10 3B 00 02 11]
            0xC5, 1, 0x03,      //Frame Rate [03]
            0xC8, 12, 0x00, 0x44, 0x06, 0x44, 0x0A, 0x08, 0x17, 0x33, 0x77, 0x44, 0x08, 0x0C,
            0xE4, 1, 0xA0,
            0xF0, 1, 0x01,			
            0x36, 1, 0x0A,      //Memory Access [00]

            0x3A, 1, 0x55,      //Interlace Pixel Format [XX]
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 120,
            0x29, 0,            //Display On
            TFTLCD_DELAY8, 25,
        };
        static const uint8_t ILI9481_CMO35_regValues[] PROGMEM = {    // 320480
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 125,
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 20,
            0xB0, 1, 0x00,
            0xD0, 3, 0x07, 0x41, 0x1D,  // Set Power [00 43 18] 07,41,1D
            0xD1, 3, 0x00, 0x1C, 0x1F,  // Set VCOM  [00 00 00] x0.825, x1.32 1C,1F
            0xD2, 2, 0x01, 0x11,        // Set Power for Normal Mode [01 22]
            0xC0, 5, 0x10, 0x3B, 0x00, 0x02, 0x11,      //Set Panel Driving [10 3B 00 02 11]
            0xC5, 1, 0x03,      //Frame Rate [03]
			0xC6, 1, 0x83, 
            0xC8, 12, 0x00, 0x26, 0x21, 0x00, 0x00, 0x1F, 0x65, 0x23, 0x77, 0x00, 0x0F, 0x00,
            0xF0, 1, 0x01,		//?
            0xE4, 1, 0xA0,      //?SETCABC on Himax
            0x36, 1, 0x48,      //Memory Access [00]
            0xB4, 1, 0x11,			

            0x3A, 1, 0x55,      //Interlace Pixel Format [XX]
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 120,
            0x29, 0,            //Display On
            TFTLCD_DELAY8, 25,
        };
        static const uint8_t ILI9481_RGB_regValues[] PROGMEM = {    // 320x480
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 125,
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 20,
            0xB0, 1, 0x00,
            0xD0, 3, 0x07, 0x41, 0x1D,  // SETPOWER [00 43 18]
            0xD1, 3, 0x00, 0x2B, 0x1F,  // SETVCOM  [00 00 00] x0.900, x1.32
            0xD2, 2, 0x01, 0x11,        // SETNORPOW for Normal Mode [01 22]
            0xC0, 6, 0x10, 0x3B, 0x00, 0x02, 0x11, 0x00,     //SETPANEL [10 3B 00 02 11]
            0xC5, 1, 0x03,      //SETOSC Frame Rate [03]
            0xC6, 1, 0x80,      //SETRGB interface control
			0xC8, 12, 0x00, 0x14, 0x33, 0x10, 0x00, 0x16, 0x44, 0x36, 0x77, 0x00, 0x0F, 0x00,
            0xF3, 2, 0x40, 0x0A,			
            0xF0, 1, 0x08,
            0xF6, 1, 0x84,
            0xF7, 1, 0x80,
            0x0C, 2, 0x00, 0x55, //RDCOLMOD
			0xB4, 1, 0x00,      //SETDISPLAY
//			0xB3, 4, 0x00, 0x01, 0x06, 0x01,  //SETGRAM simple example
			0xB3, 4, 0x00, 0x01, 0x06, 0x30,  //jpegs example
            0x36, 1, 0x48,      //Memory Access [00]
            0x3A, 1, 0x66,      //Interlace Pixel Format [XX]
			0x20, 0,            //INVOFF
//			0x21, 0,            //INVON

//            0x3A, 1, 0x55,      //Interlace Pixel Format [XX]
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 120,
            0x29, 0,            //Display On
            TFTLCD_DELAY8, 25,
        };
        init_table(ILI9481_regValues, sizeof(ILI9481_regValues));
//        init_table(ILI9481_CPT29_regValues, sizeof(ILI9481_CPT29_regValues));
//        init_table(ILI9481_PVI35_regValues, sizeof(ILI9481_PVI35_regValues));
//        init_table(ILI9481_AUO317_regValues, sizeof(ILI9481_AUO317_regValues));
//        init_table(ILI9481_CMO35_regValues, sizeof(ILI9481_CMO35_regValues));
//        init_table(ILI9481_RGB_regValues, sizeof(ILI9481_RGB_regValues));
        p16 = (int16_t *) & HEIGHT;
        *p16 = 480;
        p16 = (int16_t *) & WIDTH;
        *p16 = 320;
        break;
    case 0x9486:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | REV_SCREEN;
        static const uint8_t ILI9486_regValues[] PROGMEM = {
	    	0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 50,
            0x28, 0,            //Display Off
            0xC0, 2, 0x0d, 0x0d,        //Power Control 1 [0E 0E]
            0xC1, 2, 0x43, 0x00,        //Power Control 2 [43 00]
            0xC2, 1, 0x00,      //Power Control 3 [33]
            0xC5, 4, 0x00, 0x48, 0x00, 0x48,    //VCOM  Control 1 [00 40 00 40]
            0xB4, 1, 0x00,      //Inversion Control [00]
            0xB6, 3, 0x02, 0x02, 0x3B,  // Display Function Control [02 02 3B]
#define GAMMA9486 4
#if GAMMA9486 == 0
            // default GAMMA terrible
#elif GAMMA9486 == 1
            // GAMMA f.k.	bad		
            0xE0, 15, 0x0f, 0x31, 0x2b, 0x0c, 0x0e, 0x08, 0x4e, 0xf1, 0x37, 0x07, 0x10, 0x03, 0x0e, 0x09, 0x00,
            0xE1, 15, 0x00, 0x0e, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0f, 0x0c, 0x31, 0x36, 0x0f,
#elif GAMMA9486 == 2
            // 1.2 CPT 3.5 Inch Initial Code not bad
			0xE0, 15, 0x0F, 0x1B, 0x18, 0x0B, 0x0E, 0x09, 0x47, 0x94, 0x35, 0x0A, 0x13, 0x05, 0x08, 0x03, 0x00, 
			0xE1, 15, 0x0F, 0x3A, 0x37, 0x0B, 0x0C, 0x05, 0x4A, 0x24, 0x39, 0x07, 0x10, 0x04, 0x27, 0x25, 0x00, 
#elif GAMMA9486 == 3
            // 2.2 HSD 3.5 Inch Initial Code not bad
			0xE0, 15, 0x0F, 0x1F, 0x1C, 0x0C, 0x0F, 0x08, 0x48, 0x98, 0x37, 0x0A, 0x13, 0x04, 0x11, 0x0D, 0x00, 
			0xE1, 15, 0x0F, 0x32, 0x2E, 0x0B, 0x0D, 0x05, 0x47, 0x75, 0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00, 
#elif GAMMA9486 == 4
            // 3.2 TM  3.2 Inch Initial Code not bad
			0xE0, 15, 0x0F, 0x21, 0x1C, 0x0B, 0x0E, 0x08, 0x49, 0x98, 0x38, 0x09, 0x11, 0x03, 0x14, 0x10, 0x00, 
			0xE1, 15, 0x0F, 0x2F, 0x2B, 0x0C, 0x0E, 0x06, 0x47, 0x76, 0x37, 0x07, 0x11, 0x04, 0x23, 0x1E, 0x00, 
#elif GAMMA9486 == 5
            // 4.2 WTK 3.5 Inch Initial Code too white
			0xE0, 15, 0x0F, 0x10, 0x08, 0x05, 0x09, 0x05, 0x37, 0x98, 0x26, 0x07, 0x0F, 0x02, 0x09, 0x07, 0x00, 
			0xE1, 15, 0x0F, 0x38, 0x36, 0x0D, 0x10, 0x08, 0x59, 0x76, 0x48, 0x0A, 0x16, 0x0A, 0x37, 0x2F, 0x00, 
#endif
			0x20, 0,            // Display Inversion OFF
            0x36, 1, 0x0A,      //Memory Access
            0x3A, 1, 0x55,      //Interlace Pixel
            //            0x21, 0,            //Invert display !!!
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 50,
            0x29, 0,            //Display On
        };
        init_table(ILI9486_regValues, sizeof(ILI9486_regValues));
        p16 = (int16_t *) & HEIGHT;
        *p16 = 480;
        p16 = (int16_t *) & WIDTH;
        *p16 = 320;
        break;
    case 0x7796:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS;   //thanks to safari1
        goto common_9488;
    case 0x9488:
        _lcd_capable = AUTO_READINC | MIPI_DCS_REV1 | MV_AXIS | READ_24BITS;
      common_9488:
        static const uint8_t ILI9488_regValues_max[] PROGMEM = {        // Atmel MaxTouch
            0x01, 0,            //Soft Reset
            TFTLCD_DELAY8, 50,
            0x28, 0,            //Display Off
            0xC0, 2, 0x10, 0x10,        //Power Control 1 [0E 0E]
            0xC1, 1, 0x41,      //Power Control 2 [43]
            0xC5, 4, 0x00, 0x22, 0x80, 0x40,    //VCOM  Control 1 [00 40 00 40]
            0x36, 1, 0x68,      //Memory Access [00]
            0xB0, 1, 0x00,      //Interface     [00]
            0xB1, 2, 0xB0, 0x11,        //Frame Rate Control [B0 11]
            0xB4, 1, 0x02,      //Inversion Control [02]
            0xB6, 3, 0x02, 0x02, 0x3B,  // Display Function Control [02 02 3B] .kbv NL=480
            0xB7, 1, 0xC6,      //Entry Mode      [06]
            0x3A, 1, 0x55,      //Interlace Pixel Format [XX]
            0xF7, 4, 0xA9, 0x51, 0x2C, 0x82,    //Adjustment Control 3 [A9 51 2C 82]
            0x11, 0,            //Sleep Out
            TFTLCD_DELAY8, 150,
            0x29, 0,            //Display On
        };
        init_table(ILI9488_regValues_max, sizeof(ILI9488_regValues_max));
        p16 = (int16_t *) & HEIGHT;
        *p16 = 480;
        p16 = (int16_t *) & WIDTH;
        *p16 = 320;
        break;
    case 0xB505:                //R61505V
	case 0xC505:                //R61505W
        _lcd_capable = 0 | REV_SCREEN | READ_LOWHIGH;
        static const uint16_t R61505V_regValues[] PROGMEM = {
            0x0000, 0x0000,
            0x0000, 0x0000,
            0x0000, 0x0000,
            0x0000, 0x0001,
            0x00A4, 0x0001,     //CALB=1
            TFTLCD_DELAY, 10,
            0x0060, 0x2700,     //NL
            0x0008, 0x0808,     //FP & BP
            0x0030, 0x0214,     //Gamma settings
            0x0031, 0x3715,
            0x0032, 0x0604,
            0x0033, 0x0E16,
            0x0034, 0x2211,
            0x0035, 0x1500,
            0x0036, 0x8507,
            0x0037, 0x1407,
            0x0038, 0x1403,
            0x0039, 0x0020,
            0x0090, 0x0015,     //DIVI & RTNI
            0x0010, 0x0410,     //BT,AP
            0x0011, 0x0237,     //VC,DC0,DC1
            0x0029, 0x0046,     //VCM1
            0x002A, 0x0046,     //VCMSEL,VCM2
            // Sleep mode IN sequence
            0x0007, 0x0000,
            //0x0012, 0x0000,   //PSON=0,PON=0
            // Sleep mode EXIT sequence 
            0x0012, 0x0189,     //VRH=9,VCMR=1,PSON=0,PON=0
            0x0013, 0x1100,     //VDV
            TFTLCD_DELAY, 150,
            0x0012, 0x01B9,     //VRH=9,VCMR=1,PSON=1,PON=1 [018F]
            0x0001, 0x0100,     //SS=1 Other mode settings
            0x0002, 0x0200,     //BC0=1--Line inversion
            0x0003, 0x1030,
            0x0009, 0x0001,     //ISC=1 [0000]
            0x000A, 0x0000,     // [0000]
            //            0x000C, 0x0001,   //RIM=1 [0000]
            0x000D, 0x0000,     // [0000]
            0x000E, 0x0030,     //VEM=3 VCOM equalize [0000]
            0x0050, 0x0000,     //Display window area setting
            0x0051, 0x00EF,
            0x0052, 0x0000,
            0x0053, 0x013F,
            0x0061, 0x0001,
            0x006A, 0x0000,
            0x0080, 0x0000,
            0x0081, 0x0000,
            0x0082, 0x005F,
            0x0092, 0x0100,
            0x0093, 0x0701,
            TFTLCD_DELAY, 80,
            0x0007, 0x0100,     //BASEE=1--Display On
        };
        init_table16(R61505V_regValues, sizeof(R61505V_regValues));
        break;
    case 0x7793:
    case 0xB509:
        _lcd_capable = REV_SCREEN;
        static const uint16_t R61509V_regValues[] PROGMEM = {
            0x0000, 0x0000,
            0x0000, 0x0000,
            0x0000, 0x0000,
            0x0000, 0x0000,
            TFTLCD_DELAY, 15,
            0x0400, 0x6200,     //NL=0x31 (49) i.e. 400 rows
            0x0008, 0x0808,
            //gamma
            0x0300, 0x0C00,
            0x0301, 0x5A0B,
            0x0302, 0x0906,
            0x0303, 0x1017,
            0x0304, 0x2300,
            0x0305, 0x1700,
            0x0306, 0x6309,
            0x0307, 0x0C09,
            0x0308, 0x100C,
            0x0309, 0x2232,

            0x0010, 0x0016,     //69.5Hz         0016
            0x0011, 0x0101,
            0x0012, 0x0000,
            0x0013, 0x0001,

            0x0100, 0x0330,     //BT,AP
            0x0101, 0x0237,     //DC0,DC1,VC
            0x0103, 0x0D00,     //VDV
            0x0280, 0x6100,     //VCM
            0x0102, 0xC1B0,     //VRH,VCMR,PSON,PON
            TFTLCD_DELAY, 50,

            0x0001, 0x0100,
            0x0002, 0x0100,
            0x0003, 0x1030,     //1030
            0x0009, 0x0001,
            0x000C, 0x0000,
            0x0090, 0x8000,
            0x000F, 0x0000,

            0x0210, 0x0000,
            0x0211, 0x00EF,
            0x0212, 0x0000,
            0x0213, 0x018F,     //432=01AF,400=018F
            0x0500, 0x0000,
            0x0501, 0x0000,
            0x0502, 0x005F,     //???
            0x0401, 0x0001,     //REV=1
            0x0404, 0x0000,
            TFTLCD_DELAY, 50,

            0x0007, 0x0100,     //BASEE
            TFTLCD_DELAY, 50,

            0x0200, 0x0000,
            0x0201, 0x0000,
        };
        init_table16(R61509V_regValues, sizeof(R61509V_regValues));
        p16 = (int16_t *) & HEIGHT;
        *p16 = 400;
        break;
    }
    _lcd_rev = ((_lcd_capable & REV_SCREEN) != 0);
    setRotation(0);             //PORTRAIT
    invertDisplay(false);
}
