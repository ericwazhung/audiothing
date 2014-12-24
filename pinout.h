/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */




#ifndef __PINOUT_H__
#define __PINOUT_H__


#ifdef __AVR_ATmega328P__
 //Use pinoutTrinketPro.h *instead of this file*
 #include "pinoutTrinketPro.h"
#else




//Isn't this usually in the makefile???
/*
#define HEART_PINNUM PB1
#define HEART_PINPORT PORTB
*/

//Using the programming-header for debugging uart...
//
//
// 1  GND
// 2  V+
// 3  SCK   PB2   Rx0   (puar)   kbInput
// 4  MOSI  PB0   Tx0   (puat)   outToPC
// 5  /RST
// 6  MISO  PB1   (Heart)
//(7) GND      \ These two to make Tx available to PC while KB connected
//(8) Tx0      / on the standard programming header.


#define Rx0pin    PB2      //SCK
#define Rx0PORT   PORTB

#define Tx0pin    PB0      //MOSI
#define Tx0PORT   PORTB


#define SPI_MOSI_pin    PA1
#define SPI_MISO_pin    PA0
#define SPI_SCK_pin     PA2


//This should probably be verified; taken from writings on the 512MB card
// long after it was last in-use (a/o v50)
//
//  .=======================
//  | | N/C  ||
//  | |------||
//  | | MISO ||
//  |========||      SD-Card
//  | | GND  ||      SPI pinout
//  |========||      (SD-Card is the 'Slave' device)
//  | | SCK  ||
//  |========||
//  || 3.3V  ||
//  |========||
//  || GND   ||
//  |========||
//  | | MOSI ||
//  |========||
//  | | /CS  ||
//  |===========||
//   \  | N/C   ||
//    \=====================
//     
#define SD_CS_pin    PA3
#define SD_CS_PORT   PORTA
#define SD_MOSI_pin  SPI_MOSI_pin //PA1
#define SD_MOSI_PORT PORTA
#define SD_MISO_pin  SPI_MISO_pin //PA0
#define SD_MISO_PORT PORTA
#define SD_SCK_pin   SPI_SCK_pin //PA2
#define SD_SCK_PORT  PORTA



// The Nokia LCD used currently has 8 pins and is well-documented online...
// Pin1 on the left, looking at the back of the LCD
// I will not be using the documented-names. Instead I'll use AKA's.
//
//Num| Name | AKA  | Description                            | uC Pin
//------------------------------------------------------------------------
// 1 | VDD  | V+   |                                        |
// 2 | SCK  | SCK  |                                        | SPI_SCK/PA2
// 3 | SDIN | MOSI | Serial Data Input                      | SPI_MOSI/PA1
// 4 | D/C  | DnC  | Data/Command Select: Data=H, Command=L | PB3
// 5 | SCE  | nCS  | Serial Chip Enable (Active Low)        | PB4
// 6 | GND  | GND  |                                        |
// 7 | VOUT | ---- | VOUT >---|(---->GND (10uF)             |
// 8 | RES  | nRST | Reset (Active Low)                     | PB6



//These probably won't be used, code-wise... 
// since SPI should set them as appropriate
#define LCD_SDATA_pin   SPI_MOSI_pin
#define LCD_SDATA_PORT  SPI_MOSI_PORT
#define LCD_SCK_pin     SPI_SCK_pin
#define LCD_SCK_PORT    SPI_SCK_PORT
//These will.
#define LCD_DnC_pin     PB3
#define LCD_DnC_PORT    PORTB
#define LCD_nCS_pin     PB4
#define LCD_nCS_PORT    PORTB
#define LCD_nRST_pin    PB6
#define LCD_nRST_PORT   PORTB

//BUTTON_PIN on PA6/AIN0, when used... it's been a while.
#define BUTTON_PIN      PA6
// THIS NOTE FROM OTHER FILES, APPEARS TO BE WRONG.
// AIN1 is PA5 (positive input) tied to a voltage-divider @ VCC/2
// More correctly (assumed, a/o v30, in which this was no longer used):
// PA5 = AIN2: Tied to a voltage-divider @ VCC/2, as a reference
// PA6 = AIN0: Tied to the DAC button array, ground, and capacitor to GND
//  (eh? Button Array has two pins, AIN0 and GND
//       AIN0 is tied to ground via capacitor
//       Were we charging the capacitor via AIN0 as an output, then
//       determining the discharge time via the button's resistor?)

// In Order a/o v30, with HS_KB:
//                        ATtiny861
//                     ____________________
//                    |         |_|        |
//   PRG_MOSI / Tx0 --|  1 PB0      PA0 20 |-- SPI_MISO (SD_MISO)
// PRG_MISO / Heart --|  2 PB1      PA1 19 |-- SPI_MOSI (SD_MOSI/LCD_SDATA)
//   PRG_SCK  / Rx0 --|  3 PB2      PA2 18 |-- SPI_SCK (SD_SCK/LCD_SCK)
//          LCD_DnC --|  4 PB3      PA3 17 |-- SD_CS
//            (VCC) --|  5 VCC     AGND 16 |-- GND
//            (GND) --|  6 GND     AVCC 15 |-- VCC
//          LCD_nCS --|  7 PB4      PA4 14 |-- *N/C*
// Audio Out (OC1D) --|  8 PB5      PA5 13 |-- (AIN2: Vdiv @ VCC/2)
//         LCD_nRST --|  9 PB6      PA6 12 |-- (AIN0: Button Array) 
//           uC_RST --| 10 PB7      PA7 11 |-- Audio In (ADC6)
//                    |____________________|

#endif //Conditional inclusion of this file vs. pinoutTrinketPro.h

#endif
/* mehPL:
 *    I would love to believe in a world where licensing shouldn't be
 *    necessary; where people would respect others' work and wishes, 
 *    and give credit where it's due. 
 *    A world where those who find people's work useful would at least 
 *    send positive vibes--if not an email.
 *    A world where we wouldn't have to think about the potential
 *    legal-loopholes that others may take advantage of.
 *
 *    Until that world exists:
 *
 *    This software and associated hardware design is free to use,
 *    modify, and even redistribute, etc. with only a few exceptions
 *    I've thought-up as-yet (this list may be appended-to, hopefully it
 *    doesn't have to be):
 * 
 *    1) Please do not change/remove this licensing info.
 *    2) Please do not change/remove others' credit/licensing/copyright 
 *         info, where noted. 
 *    3) If you find yourself profiting from my work, please send me a
 *         beer, a trinket, or cash is always handy as well.
 *         (Please be considerate. E.G. if you've reposted my work on a
 *          revenue-making (ad-based) website, please think of the
 *          years and years of hard work that went into this!)
 *    4) If you *intend* to profit from my work, you must get my
 *         permission, first. 
 *    5) No permission is given for my work to be used in Military, NSA,
 *         or other creepy-ass purposes. No exceptions. And if there's 
 *         any question in your mind as to whether your project qualifies
 *         under this category, you must get my explicit permission.
 *
 *    The open-sourced project this originated from is ~98% the work of
 *    the original author, except where otherwise noted.
 *    That includes the "commonCode" and makefiles.
 *    Thanks, of course, should be given to those who worked on the tools
 *    I've used: avr-dude, avr-gcc, gnu-make, vim, usb-tiny, and 
 *    I'm certain many others. 
 *    And, as well, to the countless coders who've taken time to post
 *    solutions to issues I couldn't solve, all over the internets.
 *
 *
 *    I'd love to hear of how this is being used, suggestions for
 *    improvements, etc!
 *         
 *    The creator of the original code and original hardware can be
 *    contacted at:
 *
 *        EricWazHung At Gmail Dotcom
 *
 *    This code's origin (and latest versions) can be found at:
 *
 *        https://code.google.com/u/ericwazhung/
 *
 *    The site associated with the original open-sourced project is at:
 *
 *        https://sites.google.com/site/geekattempts/
 *
 *    If any of that ever changes, I will be sure to note it here, 
 *    and add a link at the pages above.
 *
 * This license added to the original file located at:
 * /home/meh/_avrProjects/audioThing/57-heart2/pinout.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
