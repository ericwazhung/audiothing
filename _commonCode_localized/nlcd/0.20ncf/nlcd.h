/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//nlcd 0.20ncf
// Drive that ol' B/W Nokia LCD display; the Nokia 5110/3310
// This is the uber-hacker-friendly SPI-based graphical display with
// built-in memory.
// THIS PARTICULAR VERSION: implements a *limited* font in 256 bytes of
// *eeprom* due to the original project's overstuffed program-memory.
// It may well be expanded now that we're using the atmega328p
// But likely will retain eeprom-based limited character-set as an option.
// THIS PARTICULAR VERSION: is text-only, and in-general, treats the 
// display a bit like a scrolling terminal, e.g. text can be added 
// character-by-character and will scroll to make the most-recent 
// characters visible.

//NOTE: Much of this code was derived-from the files 3310_routines.c/h
//which has the following (very limited) author-info.
//(These files are available everywhere across the internets, but don't
// seem to have specific license-notes)
//
//   ********************************************************
//   ****  Header file for 3310_routines.c  *****
//   ********************************************************
//   Controller:   ATmega32 (Clock: 1 Mhz-internal)
//   Compiler:             ImageCraft ICCAVR
//   Author:               CC Dharmani, Chennai (India)
//   Date:                 Sep 2008
//
//   Modifed by Michael Spiceland (http://tinkerish.com) to
//   pixel level functions with lcd_buffer[][].
//   Jan 2009

//0.20-1 Adding between-punctuation
//0.20 Apparently the entire "limited" font was only 190 bytes!
//     Why didn't I bother to fill up at least the 256 available?
//     It's relatively irrelevent, now, with the mega328p, but I'm not
//     ready to recode explicitely for that.
//     First Step: Generalize the math a bit... no actual additional
//     characters yet.
//0.10ncf-3 Cleanup
//0.10ncf-2 Finished an initial testMega328p with a few minor bugs
//          Updated Basic Usage
//          NLCD is NOW FUNCTIONAL.
//0.10ncf-1 Replaced testMega328p with usart_spi/testMega328p+FastSlow
//          Added _USART_SPI_HEADER_ etc.
//          Looking into Basic Usage
//0.10ncf - This is the first version moved to _commonCode 
//          from/as-of audioThing v50 (atmega328p switchover)
//          Adding notes and 3310_routines.c author-info...
//          Adding support for _commonCoded usi_spi and usart_spi


// BASIC USAGE:
// The Nokia LCD used currently has 8 pins and is well-documented online...
// Pin1 on the left, looking at the back of the LCD
// I will not be using the documented-names. Instead I'll use AKA's.
//
//Num| Name | AKA  | Description                            | uC Pin
//------------------------------------------------------------------------
// 1 | VDD  | V+   | 3.3V                                   |
// 2 | SCK  | SCK  |                                        | SPI_SCK
// 3 | SDIN | MOSI | Serial Data Input                      | SPI_MOSI
// 4 | D/C  | DnC  | Data/Command Select: Data=H, Command=L | GPO
// 5 | SCE  | nCS  | Serial Chip Enable (Active Low)        | GPO
// 6 | GND  | GND  |                                        |
// 7 | VOUT | ---- | VOUT >---|(---->GND (10uF)             |
// 8 | RES  | nRST | Reset (Active Low)                     | GPO


// main()
// {
//    spi_init();
//    nlcd_init();
//    
//    nlcd_appendCharacter('O');
//    nlcd_appendCharacter('K');
//
//    while (1)
//    {
//       if(nlcd_charactersChanged())
//          nlcd_redrawCharacters();
//
//       ...do something...
//       
//    }
// }





#ifndef __NLCD_H__
#define __NLCD_H__

#include <util/delay.h> //For delay_us/ms

#include "pinout.h" //This file needs to be in your project-directory
                    //And contain NLCD_nCS_pin, etc...
#include <avr/pgmspace.h>//progmem.h"

#include <avr/eeprom.h>

//#include "spi.h"
#ifdef __AVR_ATmega328P__
 #include _USART_SPI_HEADER_
#else
 #include _USI_SPI_HEADER_
#endif


//TODO: This needs to be changed, nlcd.c is #included at the bottom



// Active Area:  84x48 = 14x6 characters @ (5+1)x7 each

//Maximum number of bytes available for the font...
#define FONTBYTES 256
#define SCREEN_CHARS 84


#define NLCD_Select()   clrpinPORT(NLCD_nCS_pin, NLCD_nCS_PORT)

#define NLCD_Deselect() setpinPORT(NLCD_nCS_pin, NLCD_nCS_PORT)

#define NLCD_Reset() \
({ \
   clrpinPORT(NLCD_nRST_pin, NLCD_nRST_PORT); \
   _delay_ms(100); \
   setpinPORT(NLCD_nRST_pin, NLCD_nRST_PORT); \
   {}; \
})

#define NLCD_SetCommandMode() clrpinPORT(NLCD_DnC_pin, NLCD_DnC_PORT)
#define NLCD_SetDataMode()    setpinPORT(NLCD_DnC_pin, NLCD_DnC_PORT)

//Taken almost directly from 3310_routines.c
void nlcd_writeCommand(uint8_t command);
void nlcd_writeData(uint8_t data);

void nlcd_gotoXY ( unsigned char x, unsigned char y );

#define NLCD_UseExtendedCommands()  nlcd_writeCommand(0x20 | 0x01)
#define NLCD_UseBasicCommands()     nlcd_writeCommand(0x20 | 0x00)

#define NLCD_SetContrast(val) \
({ \
   NLCD_UseExtendedCommands(); \
   nlcd_writeCommand(0x80 | ((val)&0x7f)); \
   NLCD_UseBasicCommands(); \
   {}; \
})


void nlcd_clear(void);

void nlcd_init(void);

//Draws at the current position set by gotoXY...
void nlcd_drawChar(char character);

void nlcd_appendCharacter(char character);
//This is a bit misleading... appendCharacter no longer defaults to
// redrawing immediately, so this is more like fflush...
void nlcd_redrawCharacters(void);

//This returns TRUE when appendCharacter has been called, but 
//  redrawCharacters has not yet.
static __inline__ uint8_t nlcd_charactersChanged(void);

//This'll return the character at charNum, where charNum 0 is upper-left
// charNum 83 is lower-right
static __inline__ char nlcd_getChar(uint8_t charNum);

#define NLCD_GetCharFromEnd(charNum)   \
            nlcd_getChar(SCREEN_CHARS-(charNum))

//Starts writing at the specified position on the screen, where 0 is UL
// It's not particularly smart... as after text is overwritten, it will
// set the lastPos to the last character written, which might shift
// the whole screen...
// Probably best to save the charNum, then do this, then write, then reset
// (if it's < the old charNum)
// Returns the original position in the array...
static __inline__ uint8_t nlcd_setCharNum(uint8_t charNum);

#define NLCD_SetCharNumFromEnd(charNum) \
            nlcd_setCharNum(SCREEN_CHARS-(charNum))

static __inline__ void nlcd_setBufferPos(uint8_t pos);




#include "nlcd.c"

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
 * /home/meh/_avrProjects/audioThing/55-git/_commonCode_localized/nlcd/0.20ncf/nlcd.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
