/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


#ifndef __PINOUT_H__
#define __PINOUT_H__

//Using the programming-header for debugging uart and heartbeat...
// These pins can be found on the device datasheet
// For newly-supported devices (e.g. atmega328p) they can also be found in
//  e.g. _commonCode[-local]/_make/atmega328p.mk
//
//
// 1  GND
// 2  V+
// 3  SCK    Rx0   (puar)
// 4  MOSI   Tx0   (puat)
// 5  /RST
// 6  MISO   Heart


//_PGM_xxx_yyy_NAME_ is a new method, a/o atmega328p.mk
// Most devices don't yet have it implemented, so must put actual pin/port
// names here (The commented pin/port names are from another device!)
//Likewise: If you decide to use different pins than those on the
// programming-header, type them here:

#define Rx0pin    _PGM_SCK_PIN_NAME_
#define Rx0PORT   _PGM_SCK_PORT_NAME_


#define Tx0pin    _PGM_MOSI_PIN_NAME_ //PA6     //MOSI
#define Tx0PORT   _PGM_MOSI_PORT_NAME_ //PORTA



//Changing these doesn't affect which port SPI runs on (the SPI peripheral
// port, vs the USART). We're implementing it on the USART, for now.
//These definitions probably won't be used... except maybe to initialize
//pin directions/pull-ups
#define SPI_MOSI_pin    PD1 //This definition may be unnecessary...
#define SPI_MOSI_PORT   PORTD
#define SPI_MISO_pin    PD0 //This definition may be unnecessary...
#define SPI_MISO_PORT   PORTD
#define SPI_SCK_pin     PD4 //Hardcoded in usart_spi.c
#define SPI_SCK_PORT    PORTD


// The Nokia LCD used currently has 8 pins and is well-documented online...
// Pin1 on the left, looking at the back of the LCD
// I will not be using the documented-names. Instead I'll use AKA's.
//
//Num| Name | AKA  | Description                            | uC Pin
//------------------------------------------------------------------------
// 1 | VDD  | V+   |                                        |
// 2 | SCK  | SCK  |                                        | SPI_SCK/PD4
// 3 | SDIN | MOSI | Serial Data Input                      | SPI_MOSI/PD1
// 4 | D/C  | DnC  | Data/Command Select: Data=H, Command=L | PB1
// 5 | SCE  | nCS  | Serial Chip Enable (Active Low)        | PB0
// 6 | GND  | GND  |                                        |
// 7 | VOUT | ---- | VOUT >---|(---->GND (10uF)             |
// 8 | RES  | nRST | Reset (Active Low)                     | PB2

//For my own purposes, these are mapped to another header:
// 1 GND
// 2 V+
// 3 SCK  PD4
// 4 CS   PB0
// 5 Din  PD1
// 6 D/C  PB1
// 7 RST  PB2
// 8 anaButtons matrix


//These probably won't be used, code-wise... 
// since SPI should set them as appropriate
#define NLCD_SDATA_pin  SPI_MOSI_pin
#define NLCD_SDATA_PORT SPI_MOSI_PORT
#define NLCD_SCK_pin    SPI_SCK_pin
#define NLCD_SCK_PORT      SPI_SCK_PORT
//These will.
#define NLCD_DnC_pin    PB1
#define NLCD_DnC_PORT      PORTB
#define NLCD_nCS_pin    PB0
#define NLCD_nCS_PORT      PORTB
#define NLCD_nRST_pin      PB2
#define NLCD_nRST_PORT  PORTB


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
 * /home/meh/_avrProjects/audioThing/57-heart2/_commonCode_localized/nlcd/0.20ncf/testMega328p/pinout.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
