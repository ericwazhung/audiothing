/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


// anaButtons 0.10-1

//0.10-1 More prep...
//0.10 - First common-filed version... from audioThing35
//          toward sdramThing3.0-0.9
//          This hasn't really been looked-into... mainly just a copy-paste
//          from audioThing, at this point.
//          Some addition of notes.
//          makefile copied from polled_uat-0.60

#ifndef __ANABUTTONS_H__
#define __ANABUTTONS_H__
#include <avr/io.h>
#include <stdint.h>
#include _TCNTER_HEADER_

//Connecting the Button/DAC array to AIN0 (PA6) (negative input)
// AIN1 is PA5 (positive input) tied to a voltage-divider @ VCC/2
//#define BUTTON_PIN PA6



// Kinda hokey, just a number of loops...
#define CHARGE_TIME  0xf0
#define BUTTON_TIMEOUT  (250000/3)
extern uint8_t newCompTime; // = FALSE;
extern uint8_t buttonPressed;
//tcnter_t compTime = 0;

/* This is all implementation-specific...
#define NO_B            0
#define VOL_PLUS_B      1
#define VOL_MINUS_B     2
#define PLUS_B          3
#define MINUS_B         4
#define PLAY_PAUSE_B    5
#define STOP_B          6
#define FWD_B           7
#define REV_B           8
*/

//For now, inline isn't implemented...
#define ANABUTTONS_INLINEABLE 

ANABUTTONS_INLINEABLE uint8_t anaComp_wasButtonPressed(void);
ANABUTTONS_INLINEABLE uint8_t anaComp_getButton(void);
/*
//Unused...
tcnter_t anaComp_getCompTime(void);
*/
ANABUTTONS_INLINEABLE void anaComp_update(void);

ANABUTTONS_INLINEABLE int32_t anaComp_getCompTime(void);

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
 * /home/meh/_avrProjects/audioThing/57-heart2/_commonCode_localized/anaButtons/0.50/wtf/anaButtons.wrong.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
