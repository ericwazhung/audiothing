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

//Making it more pin-compatible with sdramThing3.0...
#define Tx0pin    _PGM_MOSI_PIN_NAME_ //PA6     //MOSI
#define Tx0PORT   _PGM_MOSI_PORT_NAME_ //PORTA


//adcFreeRunning currently only supports a single channel
// (Well, it may still support two, but that's been long-since undeveloped)
// We'll use ADC0 on PC0

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
 * /home/meh/_avrProjects/audioThing/55-git/_commonCode_localized/adcFreeRunning/0.10ncf/testMega328p/pinout.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
