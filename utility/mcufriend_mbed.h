#ifndef MCUFRIEND_MBED_H_
#define MCUFRIEND_MBED_H_

#include <mbed.h>

#if defined(USE_SERIAL)
#include "mcufriend_keil_spi.h"
#else

BusOut digitalL(D0, D1, D2, D3, D4, D5, D6, D7);
BusOut digitalH(D8, D9, D10, D11, D12, D13, NC, NC);
BusOut analog(A0, A1, A2, A3, A4, A5, NC, NC);

#include "pin_shield_1.h"     //shield pin macros e.g. A2_PORT, PIN_OUTPUT()
#include "pin_shield_8.h"     //macros for write_8(), read_8(), setWriteDir(), ...

// control pins as used in MCUFRIEND shields 
  #define RD_PORT A0_PORT
  #define RD_PIN  A0_PIN
  #define WR_PORT A1_PORT
  #define WR_PIN  A1_PIN
  #define CD_PORT A2_PORT
  #define CD_PIN  A2_PIN
  #define CS_PORT A3_PORT
  #define CS_PIN  A3_PIN
  #define RESET_PORT A4_PORT
  #define RESET_PIN  A4_PIN

// general purpose pin macros
 #define RD_ACTIVE  PIN_LOW(RD_PORT, RD_PIN)
 #define RD_IDLE    PIN_HIGH(RD_PORT, RD_PIN)
 #define RD_OUTPUT  PIN_OUTPUT(RD_PORT, RD_PIN)
 #define WR_ACTIVE  PIN_LOW(WR_PORT, WR_PIN)
 #define WR_IDLE    PIN_HIGH(WR_PORT, WR_PIN)
 #define WR_OUTPUT  PIN_OUTPUT(WR_PORT, WR_PIN)
 #define CD_COMMAND PIN_LOW(CD_PORT, CD_PIN)
 #define CD_DATA    PIN_HIGH(CD_PORT, CD_PIN)
 #define CD_OUTPUT  PIN_OUTPUT(CD_PORT, CD_PIN)
 #define CS_ACTIVE  PIN_LOW(CS_PORT, CS_PIN)
 #define CS_IDLE    PIN_HIGH(CS_PORT, CS_PIN)
 #define CS_OUTPUT  PIN_OUTPUT(CS_PORT, CS_PIN)
 #define RESET_ACTIVE  PIN_LOW(RESET_PORT, RESET_PIN)
 #define RESET_IDLE    PIN_HIGH(RESET_PORT, RESET_PIN)
 #define RESET_OUTPUT  PIN_OUTPUT(RESET_PORT, RESET_PIN)

#define WR_ACTIVE2  {WR_ACTIVE; WR_ACTIVE;}
#define WR_ACTIVE4  {WR_ACTIVE2; WR_ACTIVE2;}
#define WR_ACTIVE8  {WR_ACTIVE4; WR_ACTIVE4;}
#define RD_ACTIVE2  {RD_ACTIVE; RD_ACTIVE;}
#define RD_ACTIVE4  {RD_ACTIVE2; RD_ACTIVE2;}
#define RD_ACTIVE8  {RD_ACTIVE4; RD_ACTIVE4;}
#define RD_ACTIVE16 {RD_ACTIVE8; RD_ACTIVE8;}
#define WR_IDLE2  {WR_IDLE; WR_IDLE;}
#define WR_IDLE4  {WR_IDLE2; WR_IDLE2;}
#define RD_IDLE2  {RD_IDLE; RD_IDLE;}
#define RD_IDLE4  {RD_IDLE2; RD_IDLE2;}

#if 0
#elif defined(TARGET_LPC1768) //
#define WRITE_DELAY { WR_ACTIVE2; } //96MHz
#define IDLE_DELAY  { }
#define READ_DELAY  { RD_ACTIVE8; }
#define READ_IDLE   { RD_IDLE; }
#elif defined(__MK20DX128__) || defined(___MK20DX256__) // Teensy3.0 || 3.2 96MHz
#define WRITE_DELAY { WR_ACTIVE2; }
#define READ_DELAY  { RD_ACTIVE4; RD_ACTIVE; }
#elif defined(__MK64FX512__) || defined(TARGET_M4) // Teensy3.5 120MHz thanks to PeteJohno
#define WRITE_DELAY { WR_ACTIVE4; }
#define READ_DELAY  { RD_ACTIVE8; }
#elif defined(__MK66FX1M0__) || defined(TARGET_M4) // Teensy3.6 180MHz untested.   delays can possibly be reduced.
#define WRITE_DELAY { WR_ACTIVE8; }
#define READ_DELAY  { RD_ACTIVE8; RD_ACTIVE8; }
#elif defined(TARGET_M7) // Nucleo-F767 216MHz untested.   delays can possibly be reduced.
#define WRITE_DELAY { WR_ACTIVE8; WR_ACTIVE2; }
#define IDLE_DELAY  { WR_IDLE2;WR_IDLE; }
#define READ_DELAY  { RD_ACTIVE16; RD_ACTIVE16; RD_ACTIVE4; }
#define READ_IDLE   { RD_IDLE2;RD_IDLE; }
#else
//#error unspecified delays
//#define WRITE_DELAY { WR_ACTIVE2; }
//#define READ_DELAY  { RD_ACTIVE4; RD_ACTIVE; }
#define WRITE_DELAY 
#define READ_DELAY  
#endif

#if !defined(IDLE_DELAY)
#define IDLE_DELAY WR_IDLE
#endif
#if !defined(READ_IDLE)
#define READ_IDLE RD_IDLE
#endif

// General macros.   IOCLR registers are 1 cycle when optimised.
#define WR_STROBE { WR_ACTIVE; WR_IDLE; }         //PWLW=TWRL=50ns
#define RD_STROBE RD_IDLE, RD_ACTIVE, RD_ACTIVE, RD_ACTIVE   //PWLR=TRDL=150ns
#define write8(d) { write_8(d); WRITE_DELAY; WR_STROBE; IDLE_DELAY; } // STROBEs are defined later
#define write16(x)    { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst) { RD_STROBE; READ_DELAY; dst = read_8(); READ_IDLE; } // read 250ns after RD_ACTIVE goes low
#define READ_16(dst) { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }

#define CTL_INIT()   { RD_OUTPUT; WR_OUTPUT; CD_OUTPUT; CS_OUTPUT; RESET_OUTPUT; }
#define WriteCmd(x)  { CD_COMMAND; write16(x); CD_DATA; }
#define WriteData(x) { write16(x); }

#endif   //!USE_SERIAL
#endif   //MCUFRIEND_KEIL_H_

