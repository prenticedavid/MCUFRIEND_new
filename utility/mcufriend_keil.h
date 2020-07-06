#ifndef MCUFRIEND_KEIL_H_
#define MCUFRIEND_KEIL_H_

#if defined(USE_SERIAL)
#include "mcufriend_keil_spi.h"
#else
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

// General macros.   IOCLR registers are 1 cycle when optimised.
#define WR_STROBE { WR_ACTIVE; WR_IDLE; }         //PWLW=TWRL=50ns
#define RD_STROBE RD_IDLE, RD_ACTIVE, RD_ACTIVE, RD_ACTIVE   //PWLR=TRDL=150ns

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || __TARGET_ARCH_THUMB == 4 // -O2: F411@100MHz = 1.44s 
//#define WRITE_DELAY { WR_ACTIVE; WR_ACTIVE; WR_ACTIVE; WR_ACTIVE; }
//#define READ_DELAY  { RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; }
#if 0
#elif defined(MK20D5)
#warning 50Hz
#define WRITE_DELAY { } //50MHz
#define READ_DELAY  { RD_ACTIVE; }
#elif defined(STM32F103xB)
#warning 72MHz
#define WRITE_DELAY { } //72MHz
#define READ_DELAY  { RD_ACTIVE; }
#elif defined(STM32L476xx)
#warning 80MHz
#define WRITE_DELAY { WR_ACTIVE2; } //80MHz
#define READ_DELAY  { RD_ACTIVE4; }
#elif defined(__SAM3X8E__)
#warning 84MHz
#define WRITE_DELAY { WR_ACTIVE; } //84MHz
#define READ_DELAY  { RD_ACTIVE; }
#elif defined(STM32F401xE)
#warning 84MHz
#define WRITE_DELAY { WR_ACTIVE2; } //84MHz
#define READ_DELAY  { RD_ACTIVE8; }
#elif defined(STM32F411xE)
#define WRITE_DELAY { WR_ACTIVE2; WR_ACTIVE; } //100MHz
#define READ_DELAY  { RD_ACTIVE4; RD_ACTIVE2; }
#elif defined(STM32F446xx)
#warning 180MHz
#define WRITE_DELAY { WR_ACTIVE8; } //180MHz
#define IDLE_DELAY  { WR_IDLE2;WR_IDLE; }
#define READ_DELAY  { RD_ACTIVE16;}
#elif defined(STM32F767xx)
#warning 216MHz
#define WRITE_DELAY { WR_ACTIVE8; WR_ACTIVE8; } //216MHz
#define IDLE_DELAY  { WR_IDLE4;WR_IDLE4; }
#define READ_DELAY  { RD_ACTIVE16;RD_ACTIVE16;RD_ACTIVE16;}
#elif defined(STM32H743xx) //STM32H743 GPIO needs testing
#define WRITE_DELAY { WR_ACTIVE8;WR_ACTIVE2; } //F_CPU=400MHz
#define IDLE_DELAY  { WR_IDLE2;WR_IDLE; }
#define READ_DELAY  { RD_ACTIVE16;RD_ACTIVE16;RD_ACTIVE4;}
#else 
#error check specific STM32
#endif
#elif defined(__ARM_ARCH_6M__) // -O2: F072@48MHz = 5.03s
#define WRITE_DELAY { }
#define READ_DELAY  { }
#endif

#ifndef IDLE_DELAY
#define IDLE_DELAY    { WR_IDLE; }
#endif

#define write8(x)     { write_8(x); WRITE_DELAY; WR_STROBE; IDLE_DELAY; }
#define write16(x)    { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst)   { RD_STROBE; READ_DELAY; dst = read_8(); RD_IDLE2; RD_IDLE; } // read 250ns after RD_ACTIVE goes low
#define READ_16(dst)  { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }

#define CTL_INIT()   { RD_OUTPUT; WR_OUTPUT; CD_OUTPUT; CS_OUTPUT; RESET_OUTPUT; }
#define WriteCmd(x)  { CD_COMMAND; write16(x); CD_DATA; }
#define WriteData(x) { write16(x); }

#endif   //!USE_SERIAL
#endif   //MCUFRIEND_KEIL_H_
