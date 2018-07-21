//#define USE_SPECIAL             //check for custom drivers

#define WR_ACTIVE2  {WR_ACTIVE; WR_ACTIVE;}
#define WR_ACTIVE4  {WR_ACTIVE2; WR_ACTIVE2;}
#define WR_ACTIVE8  {WR_ACTIVE4; WR_ACTIVE4;}
#define RD_ACTIVE2  {RD_ACTIVE; RD_ACTIVE;}
#define RD_ACTIVE4  {RD_ACTIVE2; RD_ACTIVE2;}
#define RD_ACTIVE8  {RD_ACTIVE4; RD_ACTIVE4;}
#define RD_ACTIVE16 {RD_ACTIVE8; RD_ACTIVE8;}
#define WR_IDLE2  {WR_IDLE; WR_IDLE;}
#define WR_IDLE4  {WR_IDLE2; WR_IDLE2;}

#if defined(USE_SPECIAL)
#include "mcufriend_special.h"
#if !defined(USE_SPECIAL_FAIL)
#warning WE ARE USING A SPECIAL CUSTOM DRIVER
#endif
#endif
#if !defined(USE_SPECIAL) || defined (USE_SPECIAL_FAIL)

#if 0
//################################### UNO ##############################
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

//################################### MEGA2560 ##############################
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
#define write8(x)     { write_8(x); WR_ACTIVE2; WR_STROBE; }
#define write16(x)    { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst)   { RD_STROBE; RD_ACTIVE; dst = read_8(); RD_IDLE; RD_IDLE; }
#define READ_16(dst)  { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }
 // Shield Control macros.
#define PIN_LOW(port, pin)    (port)->PIO_CODR = (1<<(pin))
#define PIN_HIGH(port, pin)   (port)->PIO_SODR = (1<<(pin))
#define PIN_OUTPUT(port, pin) (port)->PIO_OER = (1<<(pin))

//################################### LEONARDO ##############################
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

//################################### UNO SHIELD on BOBUINO ##############################
#elif defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644P__) //UNO shield on BOBUINO
#warning regular UNO shield on BOBUINO
#define RD_PORT PORTA
#define RD_PIN  7
#define WR_PORT PORTA
#define WR_PIN  6
#define CD_PORT PORTA
#define CD_PIN  5
#define CS_PORT PORTA
#define CS_PIN  4
#define RESET_PORT PORTA
#define RESET_PIN  3

#define BMASK         0x0F              //
#define DMASK         0x6C              //
#define write_8(x)    { PORTB = (PORTB & ~BMASK) | ((x) >> 4); \
        PORTD = (PORTD & ~DMASK) | ((x) & 0x0C) | (((x) & 0x03) << 5); }
#define read_8()      ( (PINB << 4) | (PIND & 0x0C) | ((PIND & 0x60) >> 5) )
#define setWriteDir() { DDRB |=  BMASK; DDRD |=  DMASK; }
#define setReadDir()  { DDRB &= ~BMASK; DDRD &= ~DMASK; }
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
#define WRITE_DELAY { WR_ACTIVE2; }
#define READ_DELAY  { RD_ACTIVE8; RD_ACTIVE; }
#elif defined(__MK64FX512__) // Teensy3.5 120MHz thanks to PeteJohno
#define WRITE_DELAY { WR_ACTIVE4; }
#define READ_DELAY  { RD_ACTIVE8; }
#elif defined(__MK66FX1M0__) // Teensy3.6 180MHz untested.   delays can possibly be reduced.
#define WRITE_DELAY { WR_ACTIVE8; }
#define READ_DELAY  { RD_ACTIVE16; }
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
// NUCLEO:   ARDUINO_NUCLEO_xxxx from ST Core or ARDUINO_STM_NUCLEO_F103RB from MapleCore
// BLUEPILL: ARDUINO_NUCLEO_F103C8 / ARDUINO_BLUEPILL_F103C8 from ST Core or ARDUINO_GENERIC_STM32F103C from MapleCore
// MAPLE_REV3: n/a from ST Core or ARDUINO_MAPLE_REV3 from MapleCore
// ST Core:   ARDUINO_ARCH_STM32
// MapleCore: __STM32F1__
#elif defined(__STM32F1__) || defined(ARDUINO_ARCH_STM32)   //MapleCore or ST Core
#define IS_NUCLEO64 ( defined(ARDUINO_STM_NUCLEO_F103RB) \
                   || defined(ARDUINO_NUCLEO_F030R8) || defined(ARDUINO_NUCLEO_F091RC) \
                   || defined(ARDUINO_NUCLEO_F103RB) || defined(ARDUINO_NUCLEO_F303RE) \
                   || defined(ARDUINO_NUCLEO_F401RE) || defined(ARDUINO_NUCLEO_F411RE) \
                   || defined(ARDUINO_NUCLEO_F446RE) || defined(ARDUINO_NUCLEO_L053R8) \
                   || defined(ARDUINO_NUCLEO_L152RE) || defined(ARDUINO_NUCLEO_L476RG) \
                    )
// F1xx, F4xx, L4xx have different registers and styles.  General Macros
#if defined(__STM32F1__)   //weird Maple Core
#define REGS(x) regs->x
#else                      //regular ST Core
#define REGS(x) x
#endif
#define PIN_HIGH(port, pin)   (port)-> REGS(BSRR) = (1<<(pin))
#define PIN_LOW(port, pin)    (port)-> REGS(BSRR) = (1<<((pin)+16))
#define PIN_MODE2(reg, pin, mode) reg=(reg&~(0x3<<((pin)<<1)))|(mode<<((pin)<<1))
#define GROUP_MODE(port, reg, mask, val)  {port->REGS(reg) = (port->REGS(reg) & ~(mask)) | ((mask)&(val)); }

// Family specific Macros.  F103 needs ST and Maple compatibility
// note that ILI9320 class of controller has much slower Read cycles
#if 0
#elif defined(__STM32F1__) || defined(ARDUINO_NUCLEO_F103C8) || defined(ARDUINO_BLUEPILL_F103C8) || defined(ARDUINO_NUCLEO_F103RB)
#define WRITE_DELAY { }
#define READ_DELAY  { RD_ACTIVE; }
#if defined(__STM32F1__)  //MapleCore crts.o does RCC.  not understand regular syntax anyway
#define GPIO_INIT()      
#else
#define GPIO_INIT()   { RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN | RCC_APB2ENR_AFIOEN; \
        AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1;}
#endif
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

// should be easy to add F030, F091, F303, L053, ...
#elif defined(STM32F030x8)
#define WRITE_DELAY { }
#define READ_DELAY  { RD_ACTIVE; }
#define GPIO_INIT()   { RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN; }
#define PIN_OUTPUT(port, pin) PIN_MODE2((port)->MODER, pin, 0x1)

#elif defined(STM32F091xC)
#define WRITE_DELAY { }
#define READ_DELAY  { RD_ACTIVE; }
#define GPIO_INIT()   { RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN; }
#define PIN_OUTPUT(port, pin) PIN_MODE2((port)->MODER, pin, 0x1)

#elif defined(STM32F303xE)
#define WRITE_DELAY { }
#define READ_DELAY  { RD_ACTIVE; }
#define GPIO_INIT()   { RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN; \
                      /* AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1; */ }

#elif defined(STM32F401xE)
#define WRITE_DELAY { WR_ACTIVE2; }
#define READ_DELAY  { RD_ACTIVE4; }
#define GPIO_INIT()   { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN; }
#define PIN_OUTPUT(port, pin) PIN_MODE2((port)->MODER, pin, 0x1)

#elif defined(STM32F411xE)
#define WRITE_DELAY { WR_ACTIVE2; WR_ACTIVE; }
#define READ_DELAY  { RD_ACTIVE4; RD_ACTIVE2; }
#define GPIO_INIT()   { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN; }
#define PIN_OUTPUT(port, pin) PIN_MODE2((port)->MODER, pin, 0x1)

#elif defined(STM32F446xx)
#define WRITE_DELAY { WR_ACTIVE8; }
#define IDLE_DELAY  { WR_IDLE;WR_IDLE;WR_IDLE; }
#define READ_DELAY  { RD_ACTIVE16;}
#define GPIO_INIT()   { RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN; }
#define PIN_OUTPUT(port, pin) PIN_MODE2((port)->MODER, pin, 0x1)

#elif defined(STM32L053xx)
#define WRITE_DELAY { }
#define READ_DELAY  { RD_ACTIVE; }
#define GPIO_INIT()   { RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN | RCC_IOPENR_GPIOCEN; }
#define PIN_OUTPUT(port, pin) PIN_MODE2((port)->MODER, pin, 0x1)

#elif defined(STM32L152xE)
#define WRITE_DELAY { }
#define READ_DELAY  { RD_ACTIVE; }
#define GPIO_INIT()   { RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN; }
#define PIN_OUTPUT(port, pin) PIN_MODE2((port)->MODER, pin, 0x1)

#elif defined(STM32L476xx)
#define WRITE_DELAY { WR_ACTIVE2; }
#define READ_DELAY  { RD_ACTIVE4; RD_ACTIVE; }
#define GPIO_INIT()   { RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN; }
#define PIN_OUTPUT(port, pin) PIN_MODE2((port)->MODER, pin, 0x1)

#else
#error unsupported STM32
#endif

#if 0
#elif defined(ARDUINO_GENERIC_STM32F103C) || defined(ARDUINO_NUCLEO_F103C8) || defined(ARDUINO_BLUEPILL_F103C8)
#warning Uno Shield on BLUEPILL
#define RD_PORT GPIOB
//#define RD_PIN  5
#define RD_PIN  0  //hardware mod to Adapter.  Allows use of PB5 for SD Card
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

#elif IS_NUCLEO64 // Uno Shield on NUCLEO
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


#if defined(ARDUINO_NUCLEO_F103RB) || defined(ARDUINO_STM_NUCLEO_F103RB) //F103 has unusual GPIO modes
//                                 PA10,PA9,PA8                       PB10                   PB5,PB4,PB3                             PC7
#define setWriteDir() {GP_OUT(GPIOA, CRH, 0xFFF); GP_OUT(GPIOB, CRH, 0xF00); GP_OUT(GPIOB, CRL, 0xFFF000); GP_OUT(GPIOC, CRL, 0xF0000000); }
#define setReadDir()  {GP_INP(GPIOA, CRH, 0xFFF); GP_INP(GPIOB, CRH, 0xF00); GP_INP(GPIOB, CRL, 0xFFF000); GP_INP(GPIOC, CRL, 0xF0000000); }
#else      //F0xx, F3xx, F4xx, L0xx, L1xx, L4xx use MODER
//                                   PA10,PA9,PA8           PB10,PB5,PB4,PB3                      PC7
#define setWriteDir() { setReadDir(); \
                        GPIOA->MODER |=  0x150000; GPIOB->MODER |=  0x100540; GPIOC->MODER |=  0x4000; }
#define setReadDir()  { GPIOA->MODER &= ~0x3F0000; GPIOB->MODER &= ~0x300FC0; GPIOC->MODER &= ~0xC000; }
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

#ifndef IDLE_DELAY
#define IDLE_DELAY    { WR_IDLE; }
#endif

#define write8(x)     { write_8(x); WRITE_DELAY; WR_STROBE; IDLE_DELAY; }
#define write16(x)    { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst)   { RD_STROBE; READ_DELAY; dst = read_8(); RD_IDLE; RD_IDLE; }
#define READ_16(dst)  { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }

//################################### ESP32 ##############################
#elif defined(ESP32)       //regular UNO shield on TTGO D1 R32 (ESP32)
#define LCD_RD  2  //LED
#define LCD_WR  4
#define LCD_RS 15  //hard-wired to A2 (GPIO35) 
#define LCD_CS 33  //hard-wired to A3 (GPIO34)
#define LCD_RST 32 //hard-wired to A4 (GPIO36)

#define LCD_D0 12
#define LCD_D1 13
#define LCD_D2 26
#define LCD_D3 25
#define LCD_D4 17
#define LCD_D5 16
#define LCD_D6 27
#define LCD_D7 14

#define RD_PORT GPIO.out
#define RD_PIN  LCD_RD
#define WR_PORT GPIO.out
#define WR_PIN  LCD_WR
#define CD_PORT GPIO.out
#define CD_PIN  LCD_RS
#define CS_PORT GPIO.out1.val
#define CS_PIN  LCD_CS
#define RESET_PORT GPIO.out1.val
#define RESET_PIN  LCD_RST

static inline uint32_t map_8(uint32_t d)
{
    return (
               0
               | ((d & (1 << 0)) << (LCD_D0 - 0))
               | ((d & (1 << 1)) << (LCD_D1 - 1))
               | ((d & (1 << 2)) << (LCD_D2 - 2))
               | ((d & (1 << 3)) << (LCD_D3 - 3))
               | ((d & (1 << 4)) << (LCD_D4 - 4))
               | ((d & (1 << 5)) << (LCD_D5 - 5))
               | ((d & (1 << 6)) << (LCD_D6 - 6))
               | ((d & (1 << 7)) << (LCD_D7 - 7))
           );
}

static inline uint8_t map_32(uint32_t d)
{
    return (
               0
               | ((d & (1 << LCD_D0)) >> (LCD_D0 - 0))
               | ((d & (1 << LCD_D1)) >> (LCD_D1 - 1))
               | ((d & (1 << LCD_D2)) >> (LCD_D2 - 2))
               | ((d & (1 << LCD_D3)) >> (LCD_D3 - 3))
               | ((d & (1 << LCD_D4)) >> (LCD_D4 - 4))
               | ((d & (1 << LCD_D5)) >> (LCD_D5 - 5))
               | ((d & (1 << LCD_D6)) >> (LCD_D6 - 6))
               | ((d & (1 << LCD_D7)) >> (LCD_D7 - 7))
           );
}

static inline void write_8(uint16_t data)
{
    GPIO.out_w1tc = map_8(0xFF);  //could define once as DMASK
    GPIO.out_w1ts = map_8(data);
}

static inline uint8_t read_8()
{
    return map_32(GPIO.in);
}
static void setWriteDir()
{
    pinMode(LCD_D0, OUTPUT);
    pinMode(LCD_D1, OUTPUT);
    pinMode(LCD_D2, OUTPUT);
    pinMode(LCD_D3, OUTPUT);
    pinMode(LCD_D4, OUTPUT);
    pinMode(LCD_D5, OUTPUT);
    pinMode(LCD_D6, OUTPUT);
    pinMode(LCD_D7, OUTPUT);
}

static void setReadDir()
{
    pinMode(LCD_D0, INPUT);
    pinMode(LCD_D1, INPUT);
    pinMode(LCD_D2, INPUT);
    pinMode(LCD_D3, INPUT);
    pinMode(LCD_D4, INPUT);
    pinMode(LCD_D5, INPUT);
    pinMode(LCD_D6, INPUT);
    pinMode(LCD_D7, INPUT);
}

#define WRITE_DELAY { }
#define READ_DELAY  { }

#define write8(x)     { write_8(x); WRITE_DELAY; WR_STROBE; }
#define write16(x)    { uint8_t h = (x)>>8, l = x; write8(h); write8(l); }
#define READ_8(dst)   { RD_STROBE; READ_DELAY; dst = read_8(); RD_IDLE; }
#define READ_16(dst)  { uint8_t hi; READ_8(hi); READ_8(dst); dst |= (hi << 8); }

#define PIN_LOW(p, b)        (digitalWrite(b, LOW))
#define PIN_HIGH(p, b)       (digitalWrite(b, HIGH))
#define PIN_OUTPUT(p, b)     (pinMode(b, OUTPUT))

#define WR_ACTIVE  REG_WRITE(GPIO_OUT_W1TC_REG, BIT(LCD_WR)) //clear PIN_LOW(WR_PORT, WR_PIN)
#define WR_IDLE    REG_WRITE(GPIO_OUT_W1TS_REG, BIT(LCD_WR)) //set   PIN_HIGH(WR_PORT, WR_PIN)

#else
#error MCU unsupported
#endif                          // regular UNO shields on Arduino boards

#endif                          //!defined(USE_SPECIAL) || defined (USE_SPECIAL_FAIL)

#define RD_ACTIVE  PIN_LOW(RD_PORT, RD_PIN)
#define RD_IDLE    PIN_HIGH(RD_PORT, RD_PIN)
#define RD_OUTPUT  PIN_OUTPUT(RD_PORT, RD_PIN)
#ifndef WR_ACTIVE
  #define WR_ACTIVE  PIN_LOW(WR_PORT, WR_PIN)
  #define WR_IDLE    PIN_HIGH(WR_PORT, WR_PIN)
#endif
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
#define WR_STROBE { WR_ACTIVE; WRITE_DELAY; WR_IDLE; }       //PWLW=TWRL=50ns
#define RD_STROBE RD_IDLE, RD_ACTIVE, RD_ACTIVE, RD_ACTIVE      //PWLR=TRDL=150ns, tDDR=100ns

#if !defined(GPIO_INIT)
#define GPIO_INIT()
#endif
#define CTL_INIT()   { GPIO_INIT(); RD_OUTPUT; WR_OUTPUT; CD_OUTPUT; CS_OUTPUT; RESET_OUTPUT; }
#define WriteCmd(x)  { CD_COMMAND; write16(x); CD_DATA; }
#define WriteData(x) { write16(x); }
