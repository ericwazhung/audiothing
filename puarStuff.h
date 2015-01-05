/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */







#include "pinout.h"

//a/o v54: Most of the notes, here, are irrelevent, regarding the
//Stowaway Keybaord which has long-since died. They're interesting notes,
//for my memory, anyhow.
//
//In Fact, I'm not certain, but I think PUAR (UART Reception) is
//mostly, if not entirely, disabled.
// What else would it be used for, besides a keyboard?


//Thought the pull-up mighta caused a crashed-keyboard
// since reading 3.3V on the KB's Tx pin, but only receiving VCC=3.1V
// But, crashed again over-night with no-pull-up.
// Also, added a diode on the output:
// KB Tx >---|<|----> uC Rx (with pull-up)
// so with pull-up it should be near the same voltage as VCC

// It's Ironic, I'd used this keyboard crash-free for quite some time, then
// when there were some moments *really worthy* of this device, the
// keyboard began crashing.

//#define RX_NOPULLUP TRUE



extern __inline__ uint8_t puar_readInput(uint8_t puarNum)
{
   return getpinPORT(Rx0pin, Rx0PORT);
}


extern __inline__ void puar_initInput(uint8_t puarNum)
{
   //Disable the pull-up (the keyboard mightn't like 3.6V on its output)
   // (Is that why it croaked?)
   // EXCEPT: If the KB's not connected, then garbage is received
   //  Options: 
   //      Add pull-down (will constantly receive Frame Errors!)
   //      Add voltage-divider
   //      Insert series diode (at KB)
   //             KB >---|<|----> UC (pulled-up, don't forget!)
   //       Hopefully vDrop is enough to satisfy...
   // WEIRD: This's been running for DAYS without a problem.
   //        Plug/Unplug didn't fix until later... several tries
   //
   // Ultimately, as I recall, the diode method was used...
   // The keyboard has since died, but I think due to other reasons.
//THIS IS FALSE unless you've reimplemented it.
#if(defined(RX_NOPULLUP) && RX_NOPULLUP)
   clrpinPORT(Rx0pin, Rx0PORT);
   setinPORT(Rx0pin, Rx0PORT);
#else
   setinpuPORT(Rx0pin, Rx0PORT);
#endif


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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/puarStuff.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
