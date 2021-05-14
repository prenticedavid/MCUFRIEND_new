#ifndef PIN_SHIELD_8_H_
#define PIN_SHIELD_8_H_

// just provide macros for the 8-bit data bus
// i.e. write_8(), read_8(), setWriteDir(), setReadDir()


#define LPC810  810
#define LPC812  812
#define LPC1343 1343
#define LPC1768 1768
#define LPC2103 2103
#define LPC2148 2148

#define ISTARGET_NUCLEO64 (0 \
        || defined(TARGET_NUCLEO_F072RB) \
        || defined(TARGET_NUCLEO_F103RB) \
        || defined(TARGET_NUCLEO_F401RE) \
        || defined(TARGET_NUCLEO_F411RE) \
        || defined(TARGET_NUCLEO_F446RE) \
        || defined(TARGET_NUCLEO_L433RC_P) \
        || defined(TARGET_NUCLEO_L476RG) \
        )

#define ISTARGET_NUCLEO144 (0 \
        || defined(TARGET_NUCLEO_F767ZI) \
        )

//#warning Using pin_SHIELD_8.h

#if 0

#elif defined(MY_BLUEPILL) // Uno Shield on BLUEPILL_ADAPTER
#warning Uno Shield on MY_BLUEPILL_ADAPTER

// configure macros for the data pins
#define AMASK 0x060F
#define BMASK 0x00C0
#define write_8(d)    { GPIOA->BSRR = AMASK << 16; GPIOB->BSRR = BMASK << 16; \
                       GPIOA->BSRR = (((d) & 3) << 9) | (((d) & 0xF0) >> 4); \
                       GPIOB->BSRR = (((d) & 0x0C) << 4); \
                       }
#define read_8()      (((GPIOA->IDR & (3<<9)) >> 9) | ((GPIOA->IDR & (0x0F)) << 4) | ((GPIOB->IDR & (3<<6)) >> 4))

#define GROUP_MODE(port, reg, mask, val)  {port->reg = (port->reg & ~(mask)) | ((mask)&(val)); }
#define GP_OUT(port, reg, mask)           GROUP_MODE(port, reg, mask, 0x33333333)
#define GP_INP(port, reg, mask)           GROUP_MODE(port, reg, mask, 0x44444444)
//                                     PA10,PA9                     PA3-PA0                         PB7,PB6  
#define setWriteDir() {GP_OUT(GPIOA, CRH, 0xFF0); GP_OUT(GPIOA, CRL, 0xFFFF); GP_OUT(GPIOB, CRL, 0xFF000000); }
#define setReadDir()  {GP_INP(GPIOA, CRH, 0xFF0); GP_INP(GPIOA, CRL, 0xFFFF); GP_INP(GPIOB, CRL, 0xFF000000); }

#elif defined(BLUEPILL) // Uno Shield on BLUEPILL_ADAPTER
#warning Uno Shield on BLUEPILL_ADAPTER

// configure macros for the data pins
#define write_8(d)    { GPIOA->BSRR = 0x00FF << 16; GPIOA->BSRR = (d) & 0xFF; }
#define read_8()      (GPIOA->IDR & 0xFF)

#define GROUP_MODE(port, reg, mask, val)  {port->reg = (port->reg & ~(mask)) | ((mask)&(val)); }
#define GP_OUT(port, reg, mask)           GROUP_MODE(port, reg, mask, 0x33333333)
#define GP_INP(port, reg, mask)           GROUP_MODE(port, reg, mask, 0x44444444)
//                                         PA7 ..PA0
#define setWriteDir() {GP_OUT(GPIOA, CRL, 0xFFFFFFFF); }
#define setReadDir()  {GP_INP(GPIOA, CRL, 0xFFFFFFFF); }

#elif defined(ITEADMAPLE) // Uno Shield on MAPLE_REV3 board
#warning Uno Shield on MAPLE_REV3 board

#define REGS(x) x
#define GROUP_MODE(port, reg, mask, val)  {port->REGS(reg) = (port->REGS(reg) & ~(mask)) | ((mask)&(val)); }
#define GP_OUT(port, reg, mask)           GROUP_MODE(port, reg, mask, 0x33333333)
#define GP_INP(port, reg, mask)           GROUP_MODE(port, reg, mask, 0x44444444)

    // configure macros for the data pins
#define write_8(d) { \
        GPIOA->REGS(BSRR) = 0x0703 << 16; \
        GPIOB->REGS(BSRR) = 0x00E0 << 16; \
        GPIOA->REGS(BSRR) = (  ((d) & (1<<0)) << 10) \
                            | (((d) & (1<<2)) >> 2) \
                            | (((d) & (1<<3)) >> 2) \
                            | (((d) & (1<<6)) << 2) \
                            | (((d) & (1<<7)) << 2); \
        GPIOB->REGS(BSRR) = (  ((d) & (1<<1)) << 6) \
                            | (((d) & (1<<4)) << 1) \
                            | (((d) & (1<<5)) << 1); \
    }

#define read_8()  (     (   (  (GPIOA->REGS(IDR) & (1<<10)) >> 10) \
                            | ((GPIOB->REGS(IDR) & (1<<7)) >> 6) \
                            | ((GPIOA->REGS(IDR) & (1<<0)) << 2) \
                            | ((GPIOA->REGS(IDR) & (1<<1)) << 2) \
                            | ((GPIOB->REGS(IDR) & (1<<5)) >> 1) \
                            | ((GPIOB->REGS(IDR) & (1<<6)) >> 1) \
                            | ((GPIOA->REGS(IDR) & (1<<8)) >> 2) \
                            | ((GPIOA->REGS(IDR) & (1<<9)) >> 2)))

//                                 PA10,PA9,PA8                   PA1,PA0                     PB7,PB6,PB5
#define setWriteDir() {GP_OUT(GPIOA, CRH, 0xFFF); GP_OUT(GPIOA, CRL, 0xFF); GP_OUT(GPIOB, CRL, 0xFFF00000); }
#define setReadDir()  {GP_INP(GPIOA, CRH, 0xFFF); GP_INP(GPIOA, CRL, 0xFF); GP_INP(GPIOB, CRL, 0xFFF00000); }

#elif defined(NUCLEO144) || ISTARGET_NUCLEO144
#if __MBED__
#warning MBED knows everything
#elif defined(STM32F767xx)
  #include <STM32F7XX.h>
#endif

#define REGS(x) x
// configure macros for the data pins
#define DMASK ((1<<15))                         //#1
#define EMASK ((1<<13)|(1<<11)|(1<<9))          //#3, #5, #6
#define FMASK ((1<<12)|(1<<15)|(1<<14)|(1<<13)) //#0, #2, #4, #7

#define write_8(d) { \
        GPIOD->REGS(BSRR) = DMASK << 16; \
        GPIOE->REGS(BSRR) = EMASK << 16; \
        GPIOF->REGS(BSRR) = FMASK << 16; \
        GPIOD->REGS(BSRR) = (  ((d) & (1<<1)) << 14); \
        GPIOE->REGS(BSRR) = (  ((d) & (1<<3)) << 10) \
                            | (((d) & (1<<5)) << 6) \
                            | (((d) & (1<<6)) << 3); \
        GPIOF->REGS(BSRR) = (  ((d) & (1<<0)) << 12) \
                            | (((d) & (1<<2)) << 13) \
                            | (((d) & (1<<4)) << 10) \
                            | (((d) & (1<<7)) << 6); \
    }

#define read_8() (       (  (  (GPIOF->REGS(IDR) & (1<<12)) >> 12) \
                            | ((GPIOD->REGS(IDR) & (1<<15)) >> 14) \
                            | ((GPIOF->REGS(IDR) & (1<<15)) >> 13) \
                            | ((GPIOE->REGS(IDR) & (1<<13)) >> 10) \
                            | ((GPIOF->REGS(IDR) & (1<<14)) >> 10) \
                            | ((GPIOE->REGS(IDR) & (1<<11)) >> 6) \
                            | ((GPIOE->REGS(IDR) & (1<<9))  >> 3) \
                            | ((GPIOF->REGS(IDR) & (1<<13)) >> 6)))


//                                             PD15                PE13,PE11,PE9          PF15,PF14,PF13,PF12
#define setWriteDir() { setReadDir(); \
                        GPIOD->MODER |=  0x40000000; GPIOE->MODER |=  0x04440000; GPIOF->MODER |=  0x55000000; }
#define setReadDir()  { GPIOD->MODER &= ~0xC0000000; GPIOE->MODER &= ~0x0CCC0000; GPIOF->MODER &= ~0xFF000000; }
    

#elif defined(NUCLEO) || ISTARGET_NUCLEO64
#if __MBED__
#warning MBED knows everything
#elif defined(STM32F072xB)
  #include <STM32F0XX.h>
#elif defined(STM32F103xB)
  #if defined(__CC_ARM)
  #include <STM32F10X.h>
  #else
  #include <STM32F1XX.h>
  #endif
#elif defined(STM32L476xx) || defined(STM32L433xx)
  #include <STM32L4XX.h>
#elif defined(STM32F401xE) || defined(STM32F411xE) || defined(STM32F446xx)
  #include <STM32F4XX.h>
#endif
// configure macros for the data pins. -00=10.06, -O1=7.85, -O1t=7.21, -O2=7.87, -O3=7.45, -O3t=7.03
  #define write_8(d) { \
   GPIOA->BSRR = 0x0700 << 16; \
   GPIOB->BSRR = 0x0438 << 16; \
   GPIOC->BSRR = 0x0080 << 16; \
   GPIOA->BSRR = (((d) & (1<<0)) << 9) \
               | (((d) & (1<<2)) << 8) \
               | (((d) & (1<<7)) << 1); \
   GPIOB->BSRR = (((d) & (1<<3)) << 0) \
               | (((d) & (1<<4)) << 1) \
               | (((d) & (1<<5)) >> 1) \
               | (((d) & (1<<6)) << 4); \
   GPIOC->BSRR = (((d) & (1<<1)) << 6); \
    }
  #define read_8() (          (((GPIOA->IDR & (1<<9)) >> 9) \
                             | ((GPIOC->IDR & (1<<7)) >> 6) \
                             | ((GPIOA->IDR & (1<<10)) >> 8) \
                             | ((GPIOB->IDR & (1<<3)) >> 0) \
                             | ((GPIOB->IDR & (1<<5)) >> 1) \
                             | ((GPIOB->IDR & (1<<4)) << 1) \
                             | ((GPIOB->IDR & (1<<10)) >> 4) \
                             | ((GPIOA->IDR & (1<<8))  >> 1)))
// be wise to clear both MODER bits properly.
#if defined(STM32F103xB)
#define GROUP_MODE(port, reg, mask, val)  {port->reg = (port->reg & ~(mask)) | ((mask)&(val)); }
#define GP_OUT(port, reg, mask)           GROUP_MODE(port, reg, mask, 0x33333333)
#define GP_INP(port, reg, mask)           GROUP_MODE(port, reg, mask, 0x44444444)
//                                 PA10,PA9,PA8                       PB10                   PB5,PB4,PB3                             PC7
#define setWriteDir() {GP_OUT(GPIOA, CRH, 0xFFF); GP_OUT(GPIOB, CRH, 0xF00); GP_OUT(GPIOB, CRL, 0xFFF000); GP_OUT(GPIOC, CRL, 0xF0000000); }
#define setReadDir()  {GP_INP(GPIOA, CRH, 0xFFF); GP_INP(GPIOB, CRH, 0xF00); GP_INP(GPIOB, CRL, 0xFFF000); GP_INP(GPIOC, CRL, 0xF0000000); }
#else
  #define setWriteDir() { setReadDir(); \
                          GPIOA->MODER |=  0x150000; GPIOB->MODER |=  0x100540; GPIOC->MODER |=  0x4000; }
  #define setReadDir()  { GPIOA->MODER &= ~0x3F0000; GPIOB->MODER &= ~0x300FC0; GPIOC->MODER &= ~0xC000; }
#endif
    

#elif __TARGET_PROCESSOR == LPC1768 || defined(TARGET_LPC1768)
  #include <LPC17xx.h>
// configure macros for the data pins
  #define write_8(d) { \
   LPC_GPIO0->FIOPIN = (LPC_GPIO0->FIOPIN & ~0x01878003) \
                  | (((d) & (1<<0)) << 1) \
                  | (((d) & (1<<1)) >> 1) \
                  | (((d) & (1<<2)) << 22) \
                  | (((d) & (1<<3)) << 20) \
                  | (((d) & (1<<4)) << 12) \
                  | (((d) & (1<<5)) << 10) \
                  | (((d) & (1<<6)) << 11) \
                  | (((d) & (1<<7)) << 11); \
  }
  #define read_8() (            (((LPC_GPIO0->FIOPIN & (1<<1)) >> 1) \
                             | ((LPC_GPIO0->FIOPIN & (1<<0)) << 1) \
                             | ((LPC_GPIO0->FIOPIN & (1<<24)) >> 22) \
                             | ((LPC_GPIO0->FIOPIN & (1<<23)) >> 20) \
                             | ((LPC_GPIO0->FIOPIN & (1<<16)) >> 12) \
                             | ((LPC_GPIO0->FIOPIN & (1<<15)) >> 10) \
                             | ((LPC_GPIO0->FIOPIN & (1<<17)) >> 11) \
                             | ((LPC_GPIO0->FIOPIN & (1<<18)) >> 11)))
  #define setWriteDir() {LPC_GPIO0->FIODIR |=  0x01878003; }
  #define setReadDir()  {LPC_GPIO0->FIODIR &= ~0x01878003; }


#elif defined(MKL25Z4) || defined(TARGET_KL25Z)
  #include <MKL25Z4.h>
// configure macros for the data pins
#if 1
#define AMASK ((1<<13)|(1<<12)|(1<<5)|(1<<4))
#define CMASK ((1<<9)|(1<<8))
#define DMASK ((1<<5)|(1<<4))
  #define write_8(d) { \
   PTA->PCOR = AMASK; PTC->PCOR = CMASK; PTD->PCOR = DMASK; \
   PTA->PSOR =      (((d) & (1<<0)) << 13) \
                  | (((d) & (1<<3)) << 9) \
                  | (((d) & (1<<4)) >> 0) \
                  | (((d) & (1<<5)) >> 0); \
   PTC->PSOR =      (((d) & (1<<6)) << 2) \
                  | (((d) & (1<<7)) << 2); \
   PTD->PSOR =      (((d) & (1<<1)) << 4) \
                  | (((d) & (1<<2)) << 2); \
  } 
  #define read_8() (          (((PTA->PDIR & (1<<13)) >> 13) \
                             | ((PTA->PDIR & (1<<12)) >> 9) \
                             | ((PTA->PDIR & (3<<4))  >> 0) \
                             | ((PTC->PDIR & (3<<8))  >> 2) \
                             | ((PTD->PDIR & (1<<4))  >> 2) \
                             | ((PTD->PDIR & (1<<5))  >> 4)))
  #define setWriteDir() {PTA->PDDR |=  AMASK;PTC->PDDR |=  CMASK;PTD->PDDR |=  DMASK; }
  #define setReadDir()  {PTA->PDDR &= ~AMASK;PTC->PDDR &= ~CMASK;PTD->PDDR &= ~DMASK; }
#else
  #define write_8(d) { \
   PTA->PDOR = (PTA->PDOR & ~0x3030) \
                  | (((d) & (1<<0)) << 13) \
                  | (((d) & (1<<3)) << 9) \
                  | (((d) & (3<<4)) << 0); \
   PTC->PDOR = (PTC->PDOR & ~0x0300) \
                  | (((d) & (3<<6)) << 2); \
   PTD->PDOR = (PTD->PDOR & ~0x0030) \
                  | (((d) & (1<<1)) << 4) \
                  | (((d) & (1<<2)) << 2); \
  }
  #define read_8() (           (((PTA->PDIR & (1<<13)) >> 13) \
                             | ((PTA->PDIR & (1<<12)) >> 9) \
                             | ((PTA->PDIR & (3<<4))  >> 0) \
                             | ((PTC->PDIR & (3<<8))  >> 2) \
                             | ((PTD->PDIR & (1<<4))  >> 2) \
                             | ((PTD->PDIR & (1<<5))  >> 4)))
  #define setWriteDir() {PTA->PDDR |=  0x3030;PTC->PDDR |=  0x0300;PTD->PDDR |=  0x0030; }
  #define setReadDir()  {PTA->PDDR &= ~0x3030;PTC->PDDR &= ~0x0300;PTD->PDDR &= ~0x0030; }
#endif

#elif defined(MKL26Z4)
  #include <MKL26Z4.h>
// configure macros for the data pins
#define AMASK ((1<<13)|(1<<12)|(1<<5)|(1<<4))
#define CMASK ((1<<9)|(1<<8))
#define DMASK ((1<<3)|(1<<2))    //PTD5, PTD4 on KL25Z
  #define write_8(d) { \
   PTA->PCOR = AMASK; PTC->PCOR = CMASK; PTD->PCOR = DMASK; \
   PTA->PSOR =      (((d) & (1<<0)) << 13) \
                  | (((d) & (1<<3)) << 9) \
                  | (((d) & (1<<4)) >> 0) \
                  | (((d) & (1<<5)) >> 0); \
   PTC->PSOR =      (((d) & (1<<6)) << 2) \
                  | (((d) & (1<<7)) << 2); \
   PTD->PSOR =      (((d) & (1<<1)) << 1) \
                  | (((d) & (1<<2)) << 1); \
  } 
  #define read_8() (          (((PTA->PDIR & (1<<13)) >> 13) \
                             | ((PTA->PDIR & (1<<12)) >> 9) \
                             | ((PTA->PDIR & (3<<4))  >> 0) \
                             | ((PTC->PDIR & (3<<8))  >> 2) \
                             | ((PTD->PDIR & (1<<3))  >> 1) \
                             | ((PTD->PDIR & (1<<2))  >> 1)))
  #define setWriteDir() {PTA->PDDR |=  AMASK;PTC->PDDR |=  CMASK;PTD->PDDR |=  DMASK; }
  #define setReadDir()  {PTA->PDDR &= ~AMASK;PTC->PDDR &= ~CMASK;PTD->PDDR &= ~DMASK; }

#elif defined(MKL05Z4) || defined(TARGET_KL05Z)
  #include <MKL05Z4.h>
// configure macros for the data pins
  #define write_8(d) { \
   PTA->PDOR = (PTA->PDOR & ~0x1C00) \
                  | (((d) & (1<<2)) << 9) \
                  | (((d) & (1<<4)) << 6) \
                  | (((d) & (1<<5)) << 7); \
   PTB->PDOR = (PTB->PDOR & ~0x0CE0) \
                  | (((d) & (3<<0)) << 10) \
                  | (((d) & (1<<3)) << 2) \
                  | (((d) & (3<<6)) << 0); \
    }
  #define read_8() (          (((PTA->PDIR & (1<<11)) >> 9) \
                             | ((PTA->PDIR & (1<<10)) >> 6) \
                             | ((PTA->PDIR & (1<<12)) >> 7) \
                             | ((PTB->PDIR & (3<<10)) >> 10) \
                             | ((PTB->PDIR & (1<<5))  >> 2) \
                             | ((PTB->PDIR & (3<<6))  >> 0)))
  #define setWriteDir() { PTA->PDDR |=  0x1C00; PTB->PDDR |=  0x0CE0; }
  #define setReadDir()  { PTA->PDDR &= ~0x1C00; PTB->PDDR &= ~0x0CE0; }


#elif (defined(MK20D7) && defined(TEENSY)) || defined(TARGET_TEENSY3_1)
#if __MBED__
#warning MBED knows everything
#else
  #include <MK20D5.h>
#endif
// configure macros for the data pins
#define AMASK ((1<<12)|(1<<13))
#define CMASK ((1<<3))
#define DMASK ((1<<0)|(1<<2)|(1<<3)|(1<<4)|(1<<7))

  #define write_8(d) { \
   PTA->PCOR = AMASK; PTC->PCOR = CMASK; PTD->PCOR = DMASK; \
   PTA->PSOR = (((d) & (1<<3)) << 9) \
              | (((d) & (1<<4)) << 9); \
   PTC->PSOR = (((d) & (1<<1)) << 2); \
   PTD->PSOR = (((d) & (1<<0)) << 3) \
              | (((d) & (1<<2)) >> 2) \
              | (((d) & (1<<5)) << 2) \
              | (((d) & (1<<6)) >> 2) \
              | (((d) & (1<<7)) >> 5); \
  } 
  #define read_8() (          (((PTD->PDIR & (1<<3)) >> 3) \
                             | ((PTC->PDIR & (1<<3)) >> 2) \
                             | ((PTD->PDIR & (1<<0)) << 2) \
                             | ((PTA->PDIR & (1<<12)) >> 9) \
                             | ((PTA->PDIR & (1<<13)) >> 9) \
                             | ((PTD->PDIR & (1<<7))  >> 2) \
                             | ((PTD->PDIR & (1<<4))  << 2) \
                             | ((PTD->PDIR & (1<<2))  << 5)))
  #define setWriteDir() {PTA->PDDR |=  AMASK;PTC->PDDR |=  CMASK;PTD->PDDR |=  DMASK; }
  #define setReadDir()  {PTA->PDDR &= ~AMASK;PTC->PDDR &= ~CMASK;PTD->PDDR &= ~DMASK; }

#elif defined(MK20D5) || defined(TARGET_K20D50M)
  #include <MK20D5.h>
// configure macros for the data pins
#define AMASK ((1<<12)|(1<<5)|(1<<2)|(1<<1))
#define CMASK ((1<<8)|(1<<4)|(1<<3))
#define DMASK ((1<<4))
  #define write_8(d) { \
   PTA->PCOR = AMASK; PTC->PCOR = CMASK; PTD->PCOR = DMASK; \
   PTA->PSOR =      (((d) & (1<<0)) << 12) \
                  | (((d) & (1<<1)) << 1) \
                  | (((d) & (1<<2)) << 3) \
                  | (((d) & (1<<5)) >> 4); \
   PTC->PSOR =      (((d) & (1<<4)) << 4) \
                  | (((d) & (3<<6)) >> 3); \
   PTD->PSOR =      (((d) & (1<<3)) << 1); \
  } 
  #define read_8() (          (((PTA->PDIR & (1<<5)) >> 3) \
                             | ((PTA->PDIR & (1<<1)) << 4) \
                             | ((PTA->PDIR & (1<<12)) >> 12) \
                             | ((PTA->PDIR & (1<<2))  >> 1) \
                             | ((PTC->PDIR & (1<<8))  >> 4) \
                             | ((PTC->PDIR & (3<<3))  << 3) \
                             | ((PTD->PDIR & (1<<4))  >> 1)))
  #define setWriteDir() {PTA->PDDR |=  AMASK;PTC->PDDR |=  CMASK;PTD->PDDR |=  DMASK; }
  #define setReadDir()  {PTA->PDDR &= ~AMASK;PTC->PDDR &= ~CMASK;PTD->PDDR &= ~DMASK; }

#elif defined(ZERO)
  #include <samd21.h>

  #ifndef PORTA
  #define PORTA PORT->Group[0]
  #define PORTB PORT->Group[1]
  #endif
// configure macros for the data pins
#if defined(D21_XPRO)
  #define AMASK 0x00220000
  #define BMASK 0x0000C0E4
  #define write_8(d) { \
   PORTA.OUT.reg = (PORTA.OUT.reg & ~AMASK) \
                  | (((d) & (1<<5)) << 16) \
                  | (((d) & (1<<7)) << 10); \
   PORTB.OUT.reg = (PORTB.OUT.reg & ~BMASK) \
                  | (((d) & (3<<0)) << 6) \
                  | (((d) & (1<<2)) << 12) \
                  | (((d) & (1<<3)) >> 1) \
                  | (((d) & (1<<4)) << 1) \
                  | (((d) & (1<<6)) << 9); \
  }
  #define read_8() (          (((PORTA.IN.reg & (1<<21)) >> 16) \
                             | ((PORTA.IN.reg & (1<<17)) >> 10) \
                             | ((PORTB.IN.reg & (3<<6)) >> 6) \
                             | ((PORTB.IN.reg & (1<<14)) >> 12) \
                             | ((PORTB.IN.reg & (1<<2))  << 1) \
                             | ((PORTB.IN.reg & (1<<5))  >> 1) \
                             | ((PORTB.IN.reg & (1<<15))  >> 9)))
  #define setWriteDir() { \
                  PORTA.DIRSET.reg = AMASK; \
                  PORTB.DIRSET.reg = BMASK; \
                      PORTA.WRCONFIG.reg = (AMASK>>16) | (0<<22) | (0<<28) | (1<<30) | (1<<31); \
                      PORTB.WRCONFIG.reg = (BMASK & 0xFFFF) | (0<<22) | (0<<28) | (1<<30); \
                        }
  #define setReadDir()  { \
                          PORTA.DIRCLR.reg = AMASK; \
                      PORTB.DIRCLR.reg = BMASK; \
                      PORTA.WRCONFIG.reg = (AMASK>>16) | (1<<17) | (0<<28) | (1<<30) | (1<<31); \
                      PORTB.WRCONFIG.reg = (BMASK & 0xFFFF) | (1<<17) | (0<<28) | (1<<30); \
                        }       
#else
  #define DMASK 0x0030C3C0
  #define write_8(x) {PORTA.OUTCLR.reg = (DMASK); \
                                          PORTA.OUTSET.reg = (((x) & 0x0F) << 6) \
                                       | (((x) & 0x30) << 10) \
                                       | (((x) & 0xC0)<<14); }
    #define read_8()   (((PORTA.IN.reg >> 6) & 0x0F) \
                    | ((PORTA.IN.reg >> 10) & 0x30) \
                    | ((PORTA.IN.reg >> 14) & 0xC0))
  #define setWriteDir() { PORTA.DIRSET.reg = DMASK; \
                      PORTA.WRCONFIG.reg = (DMASK & 0xFFFF) | (0<<22) | (1<<28) | (1<<30); \
                      PORTA.WRCONFIG.reg = (DMASK>>16) | (0<<22) | (1<<28) | (1<<30) | (1<<31); \
                        }
  #define setReadDir()  { PORTA.DIRCLR.reg = DMASK; \
                      PORTA.WRCONFIG.reg = (DMASK & 0xFFFF) | (1<<17) | (1<<28) | (1<<30); \
                      PORTA.WRCONFIG.reg = (DMASK>>16) | (1<<17) | (1<<28) | (1<<30) | (1<<31); \
                        }       
#endif

//####################################### DUE ############################
#elif defined(__SAM3X8E__)      //regular UNO shield on DUE
 // configure macros for data bus
#define BMASK         (1<<25)
#define CMASK         (0xBF << 21)
#define write_8(x)   {  PIOB->PIO_CODR = BMASK; PIOC->PIO_CODR = CMASK; \
                        PIOB->PIO_SODR = (((x) & (1<<2)) << 23); \
                        PIOC->PIO_SODR = (((x) & (1<<0)) << 22) \
                                       | (((x) & (1<<1)) << 20) \
                                       | (((x) & (1<<3)) << 25) \
                                       | (((x) & (1<<4)) << 22) \
                                       | (((x) & (1<<5)) << 20) \
                                       | (((x) & (1<<6)) << 18) \
                                       | (((x) & (1<<7)) << 16); \
					 }

#define read_8()      ( ((PIOC->PIO_PDSR & (1<<22)) >> 22)\
                      | ((PIOC->PIO_PDSR & (1<<21)) >> 20)\
                      | ((PIOB->PIO_PDSR & (1<<25)) >> 23)\
                      | ((PIOC->PIO_PDSR & (1<<28)) >> 25)\
                      | ((PIOC->PIO_PDSR & (1<<26)) >> 22)\
                      | ((PIOC->PIO_PDSR & (1<<25)) >> 20)\
                      | ((PIOC->PIO_PDSR & (1<<24)) >> 18)\
                      | ((PIOC->PIO_PDSR & (1<<23)) >> 16)\
                      )
#define setWriteDir() { PIOB->PIO_OER = BMASK; PIOC->PIO_OER = CMASK; }
#define setReadDir()  { \
                          PMC->PMC_PCER0 = (1 << ID_PIOB)|(1 << ID_PIOC);\
						  PIOB->PIO_ODR = BMASK; PIOC->PIO_ODR = CMASK;\
						}

#else
#error MCU unselected
#endif        // MCUs

#endif     //PIN_SHIELD_8_H

