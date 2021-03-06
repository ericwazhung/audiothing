/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//tinyPLL 0.10ncf
//0.10ncf - First version, stolen from audioThing 31... (ATtiny861)
#include <util/delay.h> //for _delay_us()


//This sets up the PLL for Timer1... I think the timer still needs to be
// configured to use it

void pll_enable(void)
{
   //Stolen from LCDdirectLVDSv54:
   //Stolen from threePinIDer109t:

   //Set Timer1 to use the "asynchronous clock source" (PLL at 64MHz)
   // With phase-correct PWM (256 steps up, then back down) and CLKDIV1
   // this is 64MHz/512=125kHz
   // The benefit of such high PWM frequency is the low RC values necessary
   //  for filtering to DC.
   // "To change Timer/Counter1 to the async mode follow this procedure"
   // 1: Enable the PLL
   setbit(PLLE, PLLCSR);
   // 2: Wait 100us for the PLL to stabilize
   // (can't use dmsWait since the timer updating the dmsCount 
   //  hasn't yet been started!)
   _delay_us(100);
   //   dmsWait(1);
   // 3: Poll PLOCK until it is set...
   while(!getbit(PLOCK, PLLCSR))
   {
      asm("nop");
   }
   // 4: Set the PCKE bit to enable async mode
   setbit(PCKE, PLLCSR);

}



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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/tinyPLL/0.10ncf/tinyPLL.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
