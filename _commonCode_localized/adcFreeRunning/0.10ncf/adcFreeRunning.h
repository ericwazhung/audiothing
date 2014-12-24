/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//adcFreeRunning 0.10ncf-2
//  Free Running ADC with interrupts for one to two channels
//  Designed for audioThing on the ATtiny861, and modified for ATmega328p

// NOTE RE SAMPLE-RATE:
//See 0.10ncf-2 description, below.
// The datasheets state that a "normal conversion takes 13 cycles"
// This apparently does *NOT* mean that we can rely on free-running to
// run at a fixed frequency 1/13th of the prescaler!
// THIS HAS BEEN [MIS]USED THIS WAY FOR YEARS!
// Testing in 0.10ncf-2 suggests that the vast-majority of samples *do*
// occur at this rate, that, really, only *really low* values (e.g. 0)
// cause a dramatic change in sampling-rate. For the most-part, e.g. in
// audio, there won't be many (if any?) samples so close to the rail as to
// actually read *zero*. So, for the most part, the sample-rate does appear
// to be pretty steady.
// As I said, this has been used this way since the very early versions of
// audioThing, we are now at version 50. The effect is not noticeable over
// any other sources of noise...

//0.10ncf-2 Sample-Rate seems to increase dramatically when the value is 0
//          e.g. when measuring *near* 0, and there's noise on the line
//          causing a few Zero-Readings, then the sample-rate is a *little*
//          high, but when there's *NO* '0' readings, even if all
//          readings are 1, then it seems to sample at a steady-rate that's
//          consistent regardless of the value. (~9.2ksps)
//          If *all* readings are zero, we're talking 2.5x faster-sampling
//          than the "steady-state"
//0.10ncf-1 Two oddities:
//           Free-Running seems to sample faster when the value is lower?!
//             Unknown... Values ~ 0 give nearly 22000 samples/sec
//                        higher stabilize around 9370
//           Seems to be using the wrong reference-voltage
//             found and fixed
//0.10ncf - finally moved to _commonCode...
//          a/o audioThing50 (for the ATmega328p)
//          "CommonFiling Soon!" my ass... that note was 42 versions ago!
//0.10 - First Version, stolen and modified from threePinIDer110
//       for audioThing8
//       CommonFiling soon!

//This is OLD and probably needs revision.
// Basic Usage:
//
// main(void)
// {
//   adcFR_init();
//   while(1) {
//     int16_t adcVal = adcFR_get(< adcNum if NUM_ADCS>1 >);
//     if(adcVal >= 0)
//        do something with adcVal
//   }
// }
//
// SEE ADC_ISR_EXTERNAL for other options...


#ifndef __ADC_FREE_RUNNING_H__
#define __ADC_FREE_RUNNING_H__


//#include _ADCFREERUNNING_HEADER_
#include <avr/io.h>

//TODO: adcFreeRunning.c is #included below!



//Currently only handles 1 or 2, 2 is untested
#define NUM_ADCS  1
#define ADC_LOWSPEED FALSE


#define ADC_CALC_CYCLES 13 //This is not changeable...
#define ADC_CLKDIV 64

//These are only valid if NUM_ADCS == 2
#define DISABLE_ADC_OVERLAP_CHECK TRUE
#define REMOVE_SYNCED TRUE

void adcFR_init(void);
#if(NUM_ADCS == 1)
 // adcFR_get() returns a + value if the values are new
 //   negative if they've already been read
 int16_t adcFR_get(void);
#else
 #warning "two-channels isn't well-implemented (or recently tested)"
 #warning "adcFR_get() clears the 'new vals' variable on first-call"
 #warning "so following-calls will be negative and likely not synced!"
 int16_t adcFR_get(adcNum);
#endif


#include "adcFreeRunning.c"
#endif

//ATtiny861
//ADC 4,5,6 are on PA5,6,7 (that shift killed me!)
//ADC 3,7,9 are on PA4, PB4, PB6
//  -----------------------------------------
//   adc0 / PA0 (optionally: DI)
//   adc1 / PA1 (optionally: DO)
//   adc2 / PA2 (optionally: SCK)
//   adc3 / PA4
//   adc4 / PA5
//   adc5 / PA6
//   adc6 / PA7
//   adc7 / PB4
//   adc8 / PB5 / OC1D
//   adc9 / PB6 
//   adc10 / PB7 / RESET




// ADC ISR....
// The ADC value available corresponds to the channel selected
// two interrupts ago... since this is toggling, that means we 
// are setting the same ADC number as we're receiving the data from
//The ADC has (upon interrupt):
//  just finished a conversion and already 
//  started the next based on MUX value set up in the previous interrupt
//                                              ¦
//                       ____    ____    ____   ¦____
//  ADC_INT:           /  0   XX  1   XX  2   XX¦ 3   XX
//                  ¯¯  ¯.¯¯¯    ¯.¯¯    ¯.¯¯   ¦¯.¯¯
//                        ·______  ·_____  ·____¦  ·_____
//  ADMUX Set:   init_x  /  0     X  1    X  0  ¦ X  1
//             ¯¯.¯¯¯¯.¯  ¯¯¯¯.¯¯   ¯¯.¯¯   ¯¯.¯¦   ¯¯.¯¯
//                ·    ·_____  ·_____  ·_____  ·¦____  ._____
//  Converting ADC: x /  x    X  0    X  1    X ¦0    X  1
//                 ¯¯. ¯¯¯¯¯¯.  ¯¯¯¯¯.  ¯¯¯¯¯.  ¦¯¯¯¯.  ¯¯¯¯¯.
// <int#>_<ch#>        ·_____  ·_____  ·_____  ·¦____  ·_____  ·_____
//  ADC Data:      x  /  x_x  X  x_x  X  0_0  X ¦1_1  X  2_0  X  3_1
//                 ¯¯  ¯¯¯¯¯¯   ¯¯¯¯¯   ¯¯¯¯¯   ¦¯¯¯¯   ¯¯¯¯¯   ¯¯¯¯¯
//                                              ¦
// Thus, the data available at a given ADC interrupt
//       is from the ADMUX setting *two* interrupts prior
//       the data was sampled slightly less than one interrupt prior
//                             all data valid --v
//                                              ¦
// <int#>_<MUX# set>     ____    ____    ____   ¦____    ____   ¦____
//  ADC_INT:           /  0_0 XX  1_1 XX  2_0 XX¦ 3_1 XX  4_0 XX¦ 5_1 
//                  ¯¯  ¯¯¯¯¯    ¯¯¯¯    ¯¯¯¯   ¦¯¯¯¯    ¯¯¯¯   ¦¯¯¯¯
//  (Sampling on edges)       a       b       c ¦     d       e ¦
//  <int#>_<ADC#>       _____ ¦ _____ ¦ _____ ¦ ¦____ ¦ _____ ¦ ¦____
//  Converting:   x_0 /  x_0  X  0_0  X  1_1  X  2_0  X  3_1  X  4_0
//                 ¯¯. ¯¯¯¯¯¯.  ¯¯¯¯¯.  ¯¯¯¯¯.  ¦¯¯¯¯.  ¯¯¯¯¯.  ¦¯¯¯¯
// <int#>_<ch#>        ·_____  ·_____  ·_____  ·¦____  ·_____  ·¦____
//  ADC Data:      x  / x_0   X x_0   X 0_0   X 1_1   X 2_0   X 3_1  
//                 ¯¯  ¯¯¯¯¯¯   ¯¯¯¯¯   *¯¯¯¯   *¯¯¯¯   ¯¯¯¯¯   ¦¯¯¯¯
//                                      |       |               ¦
//                                      v_______¦_______ _______|___
//  adcVal[0]                           |  a            |  c     
//                                       ¯¯¯¯¯¯¯¦¯¯¯¯¯¯¯ ¯¯¯¯¯¯¯|¯¯¯
//                                              v_______________v__
//  adcVal[1]                                   | b             | d
//                                               *¯¯¯¯¯¯¯¯¯¯¯¯¯¯ *¯
//                                               |               |
//                                               v_______________v____
//  adcValSynced                                 | a & b         | c & d
//                                                ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯ ¯¯
//                                     



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
 * /home/meh/_avrProjects/audioThing/57-heart2/_commonCode_localized/adcFreeRunning/0.10ncf/adcFreeRunning.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
