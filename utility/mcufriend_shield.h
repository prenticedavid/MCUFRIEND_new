//#define USE_SPECIAL             //check for custom drivers
#if defined(USE_SPECIAL)
#include "mcufriend_special.h"
#if !defined(USE_SPECIAL_FAIL)
#warning WE ARE USING A SPECIAL CUSTOM DRIVER
#endif
#endif
#if !defined(USE_SPECIAL) || defined (USE_SPECIAL_FAIL)

#if 0
#elif defined(__AVR_ATmega328P__)       //regular UNO shield on UNO
#define RD_PORT PORTC
#define RD_PIN  0
#define WR_PORT PORTC
#define WR_PIN  1
#define CD_PORT PORTC
#define CD_PIN  2
#define CS_PORT PORTC
#define CS_PIN  3
#define RESET_PORT PORTC
#define RESET_PIN  4

#define BMASK         0x03              //more intuitive style for mixed Ports
#define DMASK         0xFC              //does exactly the same as previous
#define write_8(x)    { PORTB = (PORTB & ~BMASK) | ((x) & BMASK); PORTD = (PORTD & ~DMASK) | ((x) & DMASK); }
#define read_8()      ( (PINB & BMASK) | (PIND & DMASK) )
#define setWriteDir() { DDRB |=  BMASK; DDRD |=  DMASK; }
#define setReadDir()  { DDRB &= ~BMASK; DDRD &= ~DMASK; }
#define write8(x)     { write_8(x); WR_STROBE; }
#define write16(x)    { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst)   { RD_STROBE; dst = read_8(); RD_IDLE; }
#define READ_16(dst)  { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }

#define PIN_LOW(p, b)        (p) &= ~(1<<(b))
#define PIN_HIGH(p, b)       (p) |= (1<<(b))
#define PIN_OUTPUT(p, b)     *(&p-1) |= (1<<(b))

#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)       //regular UNO shield on MEGA2560
#define RD_PORT PORTF
#define RD_PIN  0
#define WR_PORT PORTF
#define WR_PIN  1
#define CD_PORT PORTF
#define CD_PIN  2
#define CS_PORT PORTF
#define CS_PIN  3
#define RESET_PORT PORTF
#define RESET_PIN  4

#define EMASK         0x38
#define GMASK         0x20
#define HMASK         0x78
#define write_8(x)   {  PORTH &= ~HMASK; PORTG &= ~GMASK; PORTE &= ~EMASK; \
                        PORTH |= (((x) & (3<<0)) << 5); \
                        PORTE |= (((x) & (3<<2)) << 2); \
                        PORTG |= (((x) & (1<<4)) << 1); \
                        PORTE |= (((x) & (1<<5)) >> 2); \
                        PORTH |= (((x) & (3<<6)) >> 3); \
					 }

#define read_8()      ( ((PINH & (3<<5)) >> 5)\
                      | ((PINE & (3<<4)) >> 2)\
                      | ((PING & (1<<5)) >> 1)\
                      | ((PINE & (1<<3)) << 2)\
                      | ((PINH & (3<<3)) << 3)\
                      )
#define setWriteDir() { DDRH |=  HMASK; DDRG |=  GMASK; DDRE |=  EMASK;  }
#define setReadDir()  { DDRH &= ~HMASK; DDRG &= ~GMASK; DDRE &= ~EMASK;  }
#define write8(x)     { write_8(x); WR_STROBE; }
#define write16(x)    { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst)   { RD_STROBE; dst = read_8(); RD_IDLE; }
#define READ_16(dst)  { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }

#define PIN_LOW(p, b)        (p) &= ~(1<<(b))
#define PIN_HIGH(p, b)       (p) |= (1<<(b))
#define PIN_OUTPUT(p, b)     *(&p-1) |= (1<<(b))

//################################# ZERO and M0_PRO ############################
#elif defined(__SAMD21G18A__)   //regular UNO shield on ZERO or M0_PRO
#include "sam.h"
 // configure macros for the control pins
#define RD_PORT PORT->Group[0]
#define RD_PIN  2
#define WR_PORT PORT->Group[1]
#define WR_PIN  8
#define CD_PORT PORT->Group[1]
#define CD_PIN  9
#define CS_PORT PORT->Group[0]
#define CS_PIN  4
#define RESET_PORT PORT->Group[0]
#define RESET_PIN  5
 // configure macros for data bus
#define DMASK 0x0030C3C0
 //  #define write_8(x) PORT->Group[0].OUT.reg = (PORT->Group[0].OUT.reg & ~DMASK)|(((x) & 0x0F) << 6)|(((x) & 0x30) << 10)|(((x) & 0xC0)<<14)
#if defined(ARDUINO_SAMD_ZERO) || defined(ARDUINO_SAMD_ZERO)   // American ZERO
#define write_8(x) {\
	PORT->Group[0].OUTCLR.reg = DMASK;\
	PORT->Group[0].OUTSET.reg = (((x) & 0x0B) << 6)\
                               |(((x) & (1<<2)) << 12)\
	                           |(((x) & (1<<4)) << 4)\
	                           |(((x) & (1<<5)) << 10)\
	                           |(((x) & 0xC0) << 14);\
                   }
#define read_8()   (((PORT->Group[0].IN.reg >> 6) & 0x0B)\
                   |((PORT->Group[0].IN.reg >> 12) & (1<<2))\
                   |((PORT->Group[0].IN.reg >> 4) &  (1<<4))\
                   |((PORT->Group[0].IN.reg >> 10) & (1<<5))\
                   |((PORT->Group[0].IN.reg >> 14) & 0xC0))
#else   //default to an M0_PRO on v1.6.5 or 1.7.6
#define write_8(x) {\
	PORT->Group[0].OUTCLR.reg = DMASK;\
	PORT->Group[0].OUTSET.reg = (((x) & 0x0F) << 6)\
                               |(((x) & 0x30) << 10)\
                               |(((x) & 0xC0) << 14);\
                   }
#define read_8()   (((PORT->Group[0].IN.reg >> 6) & 0x0F)|((PORT->Group[0].IN.reg >> 10) & 0x30)|((PORT->Group[0].IN.reg >> 14) & 0xC0))
#endif
#define setWriteDir() { PORT->Group[0].DIRSET.reg = DMASK; \
	                  PORT->Group[0].WRCONFIG.reg = (DMASK & 0xFFFF) | (0<<22) | (1<<28) | (1<<30); \
	                  PORT->Group[0].WRCONFIG.reg = (DMASK>>16) | (0<<22) | (1<<28) | (1<<30) | (1<<31); \
                        }
#define setReadDir()  { PORT->Group[0].DIRCLR.reg = DMASK; \
	                  PORT->Group[0].WRCONFIG.reg = (DMASK & 0xFFFF) | (1<<17) | (1<<28) | (1<<30); \
	                  PORT->Group[0].WRCONFIG.reg = (DMASK>>16) | (1<<17) | (1<<28) | (1<<30) | (1<<31); \
                        }
#define write8(x)     { write_8(x); WR_ACTIVE; WR_STROBE; }
#define write16(x)    { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst)   { RD_STROBE; dst = read_8(); RD_IDLE; }
#define READ_16(dst)  { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }
 // Shield Control macros.
#define PIN_LOW(port, pin)    (port).OUTCLR.reg = (1<<(pin))
#define PIN_HIGH(port, pin)   (port).OUTSET.reg = (1<<(pin))
#define PIN_OUTPUT(port, pin) (port).DIR.reg |= (1<<(pin))

//####################################### DUE ############################
#elif defined(__SAM3X8E__)      //regular UNO shield on DUE
 // configure macros for the control pins
#define RD_PORT PIOA
#define RD_PIN  16
#define WR_PORT PIOA
#define WR_PIN  24
#define CD_PORT PIOA
#define CD_PIN  23
#define CS_PORT PIOA
#define CS_PIN  22
#define RESET_PORT PIOA
#define RESET_PIN  6
 // configure macros for data bus
#define BMASK         (1<<25)
#define CMASK         (0xBF << 21)
#define write_8(x)   {  PIOB->PIO_CODR = BMASK; PIOC->PIO_CODR = CMASK; \
                        PIOC->PIO_SODR = (((x) & (1<<0)) << 22); \
                        PIOC->PIO_SODR = (((x) & (1<<1)) << 20); \
                        PIOB->PIO_SODR = (((x) & (1<<2)) << 23); \
                        PIOC->PIO_SODR = (((x) & (1<<3)) << 25); \
                        PIOC->PIO_SODR = (((x) & (1<<4)) << 22); \
                        PIOC->PIO_SODR = (((x) & (1<<5)) << 20); \
                        PIOC->PIO_SODR = (((x) & (1<<6)) << 18); \
                        PIOC->PIO_SODR = (((x) & (1<<7)) << 16); \
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
#define write8(x)     { write_8(x); WR_ACTIVE; WR_STROBE; }
//#define write8(x)     { write_8(x); WR_ACTIVE; WR_STROBE; WR_IDLE; }
#define write16(x)    { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst)   { RD_STROBE; RD_ACTIVE; dst = read_8(); RD_IDLE; RD_IDLE; }
#define READ_16(dst)  { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }
 // Shield Control macros.
#define PIN_LOW(port, pin)    (port)->PIO_CODR = (1<<(pin))
#define PIN_HIGH(port, pin)   (port)->PIO_SODR = (1<<(pin))
#define PIN_OUTPUT(port, pin) (port)->PIO_OER = (1<<(pin))

#elif defined(__AVR_ATmega32U4__)       //regular UNO shield on Leonardo
#define RD_PORT PORTF
#define RD_PIN  7
#define WR_PORT PORTF
#define WR_PIN  6
#define CD_PORT PORTF
#define CD_PIN  5
#define CS_PORT PORTF
#define CS_PIN  4
#define RESET_PORT PORTF
#define RESET_PIN  1

#define BMASK         (3<<4)
#define CMASK         (1<<6)
#define DMASK         ((1<<7)|(1<<4)|(3<<0))
#define EMASK         (1<<6)
static inline                   //hope we use r24
void write_8(uint8_t x)
{
    PORTB &= ~BMASK;
    PORTC &= ~CMASK;
    PORTD &= ~DMASK;
    PORTE &= ~EMASK;
    PORTB |= (((x) & (3 << 0)) << 4);
    PORTD |= (((x) & (1 << 2)) >> 1);
    PORTD |= (((x) & (1 << 3)) >> 3);
    PORTD |= (((x) & (1 << 4)) << 0);
    PORTC |= (((x) & (1 << 5)) << 1);
    PORTD |= (((x) & (1 << 6)) << 1);
    PORTE |= (((x) & (1 << 7)) >> 1);
}

#define read_8()      ( ((PINB & (3<<4)) >> 4)\
| ((PIND & (1<<1)) << 1)\
| ((PIND & (1<<0)) << 3)\
| ((PIND & (1<<4)) >> 0)\
| ((PINC & (1<<6)) >> 1)\
| ((PIND & (1<<7)) >> 1)\
| ((PINE & (1<<6)) << 1)\
)
#define setWriteDir() { DDRB |=  BMASK; DDRC |=  CMASK; DDRD |=  DMASK; DDRE |=  EMASK;  }
#define setReadDir()  { DDRB &= ~BMASK; DDRC &= ~CMASK; DDRD &= ~DMASK; DDRE &= ~EMASK;  }
#define write8(x)     { write_8(x); WR_STROBE; }
#define write16(x)    { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst)   { RD_STROBE; dst = read_8(); RD_IDLE; }
#define READ_16(dst)  { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }

#define PIN_LOW(p, b)        (p) &= ~(1<<(b))
#define PIN_HIGH(p, b)       (p) |= (1<<(b))
#define PIN_OUTPUT(p, b)     *(&p-1) |= (1<<(b))

//####################################### TEENSY ############################
#elif defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) // regular UNO shield on a Teensy 3.x
#warning regular UNO shield on a Teensy 3.x

#if defined(__MK20DX128__) || defined(__MK20DX256__) // Teensy3.0 || 3.2 96MHz
#define WRITE_DELAY { WR_ACTIVE; WR_ACTIVE; }
#define READ_DELAY  { RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; }
#elif defined(__MK64FX512__) // Teensy3.5 120MHz thanks to PeteJohno
#define WRITE_DELAY { WR_ACTIVE; WR_ACTIVE; WR_ACTIVE; WR_ACTIVE; }
#define READ_DELAY  { RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; }
#elif defined(__MK66FX1M0__) // Teensy3.6 180MHz untested.   delays can possibly be reduced.
#define WRITE_DELAY { WR_ACTIVE; WR_ACTIVE; WR_ACTIVE; WR_ACTIVE; WR_ACTIVE; WR_ACTIVE; WR_ACTIVE; WR_ACTIVE; }
#define READ_DELAY  { RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; }
#else
#error unspecified delays
#endif

#define RD_PORT GPIOD
#define RD_PIN 1
#define WR_PORT GPIOC
#define WR_PIN 0
#define CD_PORT GPIOB
#define CD_PIN 0
#define CS_PORT GPIOB
#define CS_PIN 1
#define RESET_PORT GPIOB
#define RESET_PIN 3

// configure macros for the data pins
#define AMASK ((1<<12)|(1<<13))
#define CMASK ((1<<3))
#define DMASK ((1<<0)|(1<<2)|(1<<3)|(1<<4)|(1<<7))

#define write_8(d) { \
        GPIOA_PCOR = AMASK; GPIOC_PCOR = CMASK; GPIOD_PCOR = DMASK; \
        GPIOA_PSOR = (((d) & (1 << 3)) << 9) \
                     | (((d) & (1 << 4)) << 9); \
        GPIOC_PSOR = (((d) & (1 << 1)) << 2); \
        GPIOD_PSOR = (((d) & (1 << 0)) << 3) \
                     | (((d) & (1 << 2)) >> 2) \
                     | (((d) & (1 << 5)) << 2) \
                     | (((d) & (1 << 6)) >> 2) \
                     | (((d) & (1 << 7)) >> 5); \
        }
#define read_8() ((((GPIOD_PDIR & (1<<3)) >> 3) \
                   | ((GPIOC_PDIR & (1 << 3)) >> 2) \
                   | ((GPIOD_PDIR & (1 << 0)) << 2) \
                   | ((GPIOA_PDIR & (1 << 12)) >> 9) \
                   | ((GPIOA_PDIR & (1 << 13)) >> 9) \
                   | ((GPIOD_PDIR & (1 << 7)) >> 2) \
                   | ((GPIOD_PDIR & (1 << 4)) << 2) \
                   | ((GPIOD_PDIR & (1 << 2)) << 5)))
#define setWriteDir() {GPIOA_PDDR |= AMASK;GPIOC_PDDR |= CMASK;GPIOD_PDDR |= DMASK; }
#define setReadDir() {GPIOA_PDDR &= ~AMASK;GPIOC_PDDR &= ~CMASK;GPIOD_PDDR &= ~DMASK; }
#define write8(x) { write_8(x); WRITE_DELAY; WR_STROBE; } //PJ adjusted
#define write16(x) { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst) { RD_STROBE; READ_DELAY; dst = read_8(); RD_IDLE; } //PJ adjusted
#define READ_16(dst) { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }
//#define GPIO_INIT() {SIM_SCGC5 |= 0x3E00;}  //PORTA-PORTE
#define GPIO_INIT() {for (int i = 2; i <= 9; i++) pinMode(i, OUTPUT); for (int i = A0; i <= A4; i++) pinMode(i, OUTPUT);}

#define PASTE(x, y) x ## y

#define PIN_LOW(port, pin) PASTE(port, _PCOR) = (1<<(pin))
#define PIN_HIGH(port, pin) PASTE(port, _PSOR) = (1<<(pin))
#define PIN_OUTPUT(port, pin) PASTE(port, _PDDR) |= (1<<(pin))

//####################################### STM32 ############################
#elif defined(__STM32F1__) || defined(STM32F103xB) || defined(STM32L476xx) // MAPLECORE or STM32CORE 

#if 0
#elif defined(STM32L476xx)
#define WRITE_DELAY { WR_ACTIVE; WR_ACTIVE; }
#define READ_DELAY  { RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; RD_ACTIVE; }
#define REGS(x) x
#define PIN_MODE2(reg, pin, mode) reg=(reg&~(0x3<<((pin)<<1)))|(mode<<((pin)<<1))
#define GPIO_INIT()   { RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN; }
#define PIN_OUTPUT(port, pin) PIN_MODE2((port)->MODER, pin, 0x1)
#elif defined(ARDUINO_NUCLEO_F103C8) || defined(ARDUINO_NUCLEO_F103RB)   //regular CMSIS libraries
#define REGS(x) x
#define GPIO_INIT()   { RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN | RCC_APB2ENR_AFIOEN; \
        AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1;}
#else                                                                  //weird Maple libraries
#define REGS(x) regs->x
#endif
#if defined(__STM32F1__) || defined(STM32F103xB)
#define WRITE_DELAY { }
#define READ_DELAY  { RD_ACTIVE; }
#define GROUP_MODE(port, reg, mask, val)  {port->REGS(reg) = (port->REGS(reg) & ~(mask)) | ((mask)&(val)); }
#define GP_OUT(port, reg, mask)           GROUP_MODE(port, reg, mask, 0x33333333)
#define GP_INP(port, reg, mask)           GROUP_MODE(port, reg, mask, 0x44444444)
#define PIN_OUTPUT(port, pin) {\
        if (pin < 8) {GP_OUT(port, CRL, 0xF<<((pin)<<2));} \
        else {GP_OUT(port, CRH, 0xF<<((pin&7)<<2));} \
    }
#define PIN_INPUT(port, pin) { \
        if (pin < 8) { GP_INP(port, CRL, 0xF<<((pin)<<2)); } \
        else { GP_INP(port, CRH, 0xF<<((pin&7)<<2)); } \
    }
#endif
#define PIN_HIGH(port, pin)   (port)-> REGS(BSRR) = (1<<(pin))
#define PIN_LOW(port, pin)    (port)-> REGS(BSRR) = (1<<((pin)+16))

#if 0
#elif defined(ARDUINO_GENERIC_STM32F103C) || defined(ARDUINO_NUCLEO_F103C8)
#warning Uno Shield on BLUEPILL
#define RD_PORT GPIOB
#define RD_PIN  5
#define WR_PORT GPIOB
#define WR_PIN  6
#define CD_PORT GPIOB
#define CD_PIN  7
#define CS_PORT GPIOB
#define CS_PIN  8
#define RESET_PORT GPIOB
#define RESET_PIN  9

// configure macros for the data pins
#define write_8(d)    { GPIOA->REGS(BSRR) = 0x00FF << 16; GPIOA->REGS(BSRR) = (d) & 0xFF; }
#define read_8()      (GPIOA->REGS(IDR) & 0xFF)
//                                         PA7 ..PA0
#define setWriteDir() {GP_OUT(GPIOA, CRL, 0xFFFFFFFF); }
#define setReadDir()  {GP_INP(GPIOA, CRL, 0xFFFFFFFF); }

#elif defined(ARDUINO_STM_NUCLEO_F103RB) || defined(ARDUINO_NUCLEO_F103RB) || defined(ARDUINO_NUCLEO_L476RG) // Uno Shield on NUCLEO
#warning Uno Shield on NUCLEO
#define RD_PORT GPIOA
#define RD_PIN  0
#define WR_PORT GPIOA
#define WR_PIN  1
#define CD_PORT GPIOA
#define CD_PIN  4
#define CS_PORT GPIOB
#define CS_PIN  0
#define RESET_PORT GPIOC
#define RESET_PIN  1

// configure macros for the data pins
#define write_8(d) { \
        GPIOA->REGS(BSRR) = 0x0700 << 16; \
        GPIOB->REGS(BSRR) = 0x0438 << 16; \
        GPIOC->REGS(BSRR) = 0x0080 << 16; \
        GPIOA->REGS(BSRR) = (  ((d) & (1<<0)) << 9) \
                            | (((d) & (1<<2)) << 8) \
                            | (((d) & (1<<7)) << 1); \
        GPIOB->REGS(BSRR) = (  ((d) & (1<<3)) << 0) \
                            | (((d) & (1<<4)) << 1) \
                            | (((d) & (1<<5)) >> 1) \
                            | (((d) & (1<<6)) << 4); \
        GPIOC->REGS(BSRR) = (  ((d) & (1<<1)) << 6); \
    }

#define read_8() (       (  (  (GPIOA->REGS(IDR) & (1<<9)) >> 9) \
                            | ((GPIOC->REGS(IDR) & (1<<7)) >> 6) \
                            | ((GPIOA->REGS(IDR) & (1<<10)) >> 8) \
                            | ((GPIOB->REGS(IDR) & (1<<3)) >> 0) \
                            | ((GPIOB->REGS(IDR) & (1<<5)) >> 1) \
                            | ((GPIOB->REGS(IDR) & (1<<4)) << 1) \
                            | ((GPIOB->REGS(IDR) & (1<<10)) >> 4) \
                            | ((GPIOA->REGS(IDR) & (1<<8))  >> 1)))


#if defined(ARDUINO_NUCLEO_L476RG)
//                                   PA10,PA9,PA8           PB10,PB5,PB4,PB3                      PC7
#define setWriteDir() { setReadDir(); \
                        GPIOA->MODER |=  0x150000; GPIOB->MODER |=  0x100540; GPIOC->MODER |=  0x4000; }
#define setReadDir()  { GPIOA->MODER &= ~0x3F0000; GPIOB->MODER &= ~0x300FC0; GPIOC->MODER &= ~0xC000; }
#else
//                                 PA10,PA9,PA8                       PB10                   PB5,PB4,PB3                             PC7
#define setWriteDir() {GP_OUT(GPIOA, CRH, 0xFFF); GP_OUT(GPIOB, CRH, 0xF00); GP_OUT(GPIOB, CRL, 0xFFF000); GP_OUT(GPIOC, CRL, 0xF0000000); }
#define setReadDir()  {GP_INP(GPIOA, CRH, 0xFFF); GP_INP(GPIOB, CRH, 0xF00); GP_INP(GPIOB, CRL, 0xFFF000); GP_INP(GPIOC, CRL, 0xF0000000); }
#endif

#elif defined(ARDUINO_MAPLE_REV3) // Uno Shield on MAPLE_REV3 board
#warning Uno Shield on MAPLE_REV3 board
#define RD_PORT GPIOC
#define RD_PIN  0
#define WR_PORT GPIOC
#define WR_PIN  1
#define CD_PORT GPIOC
#define CD_PIN  2
#define CS_PORT GPIOC
#define CS_PIN  3
#define RESET_PORT GPIOC
#define RESET_PIN  4

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

#else
#error REGS group
#endif

#define write8(x)     { write_8(x); WRITE_DELAY; WR_STROBE; }
#define write16(x)    { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst)   { RD_STROBE; READ_DELAY; dst = read_8(); RD_IDLE; }
#define READ_16(dst)  { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }

#else
#error MCU unsupported
#endif                          // regular UNO shields on Arduino boards

#endif                          //!defined(USE_SPECIAL) || defined (USE_SPECIAL_FAIL)

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

 // General macros.   IOCLR registers are 1 cycle when optimised.
#define WR_STROBE { WR_ACTIVE;  delay_us(1); WR_IDLE; }       //PWLW=TWRL=50ns
#define RD_STROBE RD_IDLE, RD_ACTIVE, RD_ACTIVE, RD_ACTIVE      //PWLR=TRDL=150ns, tDDR=100ns

#if !defined(GPIO_INIT)
#define GPIO_INIT()
#endif
#define CTL_INIT()   { GPIO_INIT(); RD_OUTPUT; WR_OUTPUT; CD_OUTPUT; CS_OUTPUT; RESET_OUTPUT; }
#define WriteCmd(x)  { CD_COMMAND; write16(x); }
#define WriteData(x) { CD_DATA; write16(x); }
