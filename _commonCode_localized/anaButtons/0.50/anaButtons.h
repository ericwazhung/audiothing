/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


// anaButtons 0.50-9

//0.50-9  a/o audioThing54, Adding wiring-information for a matrix-keypad
//        and a drawing showing using a Digital GPIO
//0.50-8  a/o audioThing50 (atmega328p)
//        Adding a test for the 328
//0.50-7  a/o audioThing44:
//        Looking into non-blocking... starting a test
//        Simple switchover, just don't set BLOCKING true, and it seems to
//        work fine. Not *quite* as precise, but not bad.
//        To Consider:
//           testTiny861+NonBlocking + nokia keypad gives a maximum
//           buttonTime of ~350... as I recall, that's TCNTs
//           That's 16MHz/CLKDIV64 = 250000 tcnts/sec
//           That's 1.4ms to sample the longest duration, with blocking.
//           That's not too bad, really.
//           As I recall, it only blocks between charge/measure when a
//           button's detected, so it should seldomly block longer than
//           this... (if precision is desired)
//           e.g. if sampling audio at 44.1khz, we'd only need a buffer
//           large enough to handle 62 samples during this blocking period
//           (and enough time to write them, so who knows)
//0.50-6  More notes re: noise... in anaButtons.c where it claimed "the
//        idea being that the resistance measured can never be *lower* than
//        the resistor-value"-ish, which isn't true when noise is involved,
//        which could cause a threshold-crossing quite a bit earlier in the
//        capacitor's decay...
//0.50-5  Adding note re: noise...
//        Also Basic Usage
//0.50-4  Changed timeouts, and maybe some other things
//        + notes
//0.50-3  Cleanup + CLI test...
//0.50-2  Moving While outside states, in order to handle switching state
//        from RELEASE_CHARGE -> CLOCK_DISCHARGE within the same
//        update-call (when blocking)
//0.50-1  Total restructure... 
//0.50    Restructure based on 0.45b-5 ideas...
//        Not yet there, just did some cleanup and notes
//0.45b-5 -4 was also a nogo, reverting again to -2
//        Taking a new tactic...
//        The idea behind -3 and -4 was to loop until the threshold is
//        crossed... so at least one flaw in the design was forgetting that
//        the threshold won't be crossed for *quite some time* when the
//        button is *not* pressed.
//        Looking into stopping sampling after timeout and restarting
//        rather than waiting for it to cross the threshold.
//        There may be some flaws here, as well...
//        At the very least, it means we'll have a high fixed-frequency
//        triangle-ish wave constantly running.
//
//        Instead of a new implementation, just did some cleanup...
//        new changes will go in 0.50
//
//0.45b-4 -3 was a nogo, reverting to -2 and trying again.
//0.45b-3 Duh, just do it blocking... "b"
//0.45b-2 + measurementCount
//0.45b-1 minSampleTime to correspond with minButtonTime...
//        + testCode including random delays
//0.45b after 0.45, straight-from 0.40-2
//      New idea... If there's a big delay between updates, disregard the
//      sample... 
//0.40-2 wtf... PUAT_INLINE in anaButtons.mk?!
//       and no bithandler...

//0.40-1 updating test program, a few changes, and now using puat0.70
//0.40 Moving minButtonTime stuff to here (from main)
//     we'll call it anaButtons_getDebounced()
//0.30 Switching "AnaComp" to "AnaButtons" etc...
//     Plus cleanup
//     Plus notes on Sony remote
//0.20-1 Adding info about the Sony headphone remote...
//       Discovered a .swp file for anaButtons.h and recovered
//       This version *Untested* but the only changes should be comments
//0.20- adding functionality on *any* digital I/O pin (don't need the
//      analog-comparator!)
//      Add this to your makefile:
//      CFLAGS += -D'ANABUTTONS_DIGITALIO=TRUE'
//      And choose any pin/port for use
//0.10-somn Got it working.
//0.10-1 More prep...
//0.10 - First common-filed version... from audioThing35
//          toward sdramThing3.0-0.9
//          This hasn't really been looked-into... mainly just a copy-paste
//          from audioThing, at this point.
//          Some addition of notes.
//          makefile copied from polled_uat-0.60



// Basic Usage:
// (See tests)
//
// CFLAGS += -D'ANAB_BLOCKING=TRUE'
//
// main()
//   while()
//      int32_t buttonTimeVal = anaButtons_getDebounced();
//      if (buttonTimeVal >= 0)
//          printf("button detected: %d TCNTs", buttonTimeVal);
//
// ...
// NOTE: It ain't a joke, noise can really mess with this...
// Probably ideal to have a shielded cable to the buttons, keep the
// capacitor close to the input pin, who knows what else...
// The SONY remote seems pretty stable normally, but having a lead off a
// nearby pin for testing, its pulsing caused threshold detections *way*
// different than the norm... e.g. the PLUS button is normally 143, but
// with the "red" wire nearby measurements were varying from 118-147.
// (118 overlaps with the next largest button VOL+ which was normally 122)
//
// Disconnecting the "transmitting antenna" seems to have cleared that up.
//
// Also, not really considered yet, this system probably relies on V+ being
// consistent... maybe best if V+ is run off a local voltage-regulator, or
// maybe using a zener regulator for charging at the pin?
// Hasn't been a problem yet, but I'm using the same hardware regularly.


#ifndef __ANABUTTONS_H__
#define __ANABUTTONS_H__
#include <avr/io.h>
#include <stdint.h>
#include _TCNTER_HEADER_
#include _BITHANDLING_HEADER_


// Kinda hokey, just a number of loops...
#define CHARGE_TIME  0xf0
//#define ANAB_UNPRESSED_TIMEOUT (250000) // 16MHz/CLKDIV64 -> 1s
//Just somewhat randomly chose that value, it seemed shorter than the
//unloaded-capacitor-discharge time on my 'scope, but apparently I was 
//mistaken.
//
// /3 here is rather arbitrary, just wanted something definitely smaller
// than the unpressed-timeout and definitely larger than the maximum
// measured button-value.
//#define ANAB_BUTTON_TIMEOUT  (ANAB_UNPRESSED_TIMEOUT/3)

//Ideally, the unpressed-timeout would be chosen such that the capacitor
//can't discharge all the way down to the threshold voltage when no button
//is pressed... Otherwise (with blocking) we'll get a repeated case where
//it will detect what looks like a cut-short unpressed-timeout, which looks
//like a button-press... then it'll test for a button-press only to get a
//button-time-out and cycle again. The functions will return values that
//look fine, but the result (again with BLOCKING) is that quite a bit of
//time is spent searching for a button-press that doesn't exist.
//
//Longest button-press measurement is <200 tcnts...
#define ANAB_UNPRESSED_TIMEOUT   (2000)   // 16MHz/CLKDIV64 -> 1s
#define ANAB_BUTTON_TIMEOUT  (ANAB_UNPRESSED_TIMEOUT/3)

//extern uint8_t newCompTime;
//extern uint8_t buttonPressed;

//For now, inline isn't implemented...
#define ANABUTTONS_INLINEABLE 

//ANABUTTONS_INLINEABLE uint8_t anaButtons_wasButtonPressed(void);
//ANABUTTONS_INLINEABLE uint8_t anaButtons_getButton(void);

#define ANABUTTONS_NOBUTTON   INT32_MIN


//This could probably be avoided by using getDebounced...
//If the value is positive, it's a measurement
// Negative is a non-measurement
#define ANAB_BUTTON_RELEASED     (INT32_MIN+1)
#define ANAB_NOTHING_TO_REPORT   (ANABUTTONS_NOBUTTON)
//Dun think this is particularly well-implemented, yet.
#define ANAB_SAMPLING            (ANAB_BUTTON_RELEASED+1)

ANABUTTONS_INLINEABLE int32_t anaButtons_update(void);
//ANABUTTONS_INLINEABLE int32_t anaButtons_getCompTime(void);


//This could be the only necessary function to call from user-code...
//NYI:
//#define ANABUTTONS_MEASURING   (ANABUTTONS_NOBUTTON+1)
ANABUTTONS_INLINEABLE int32_t anaButtons_getDebounced(void);

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
 * /home/meh/_avrProjects/audioThing/57-heart2/_commonCode_localized/anaButtons/0.50/anaButtons.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
