/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


#include "spi.h"




//Not safe to use this after the timer's being used by other things...
// also it halts processing during the communication
//The above note regarding the original spi_sd_ functions...
// still may be relevent to spi in general...
uint8_t spi_transferByteWithTimer(uint8_t txByte);
uint8_t spi_transferByte(uint8_t txByte);




static __inline__
void spi_StrobeClockAndShift(void)
   __attribute__((__always_inline__));

//This would make more sense as a #define for USICR's value
//  but I want to keep all the notes here...
void spi_StrobeClockAndShift(void)
{
   USICR =
      //Set the USI port to three-wire mode
      (0<<USIWM1) | (1<<USIWM0)
      //Set the SPI port to use Timer0 compare-match for clocking
      //NOGO doesn't output on SCK pin
      //    | (0<<USICS1) | (1<<USICS0);
      //These three bits aren't exactly independent...
      //Set the SPI port to clock in data on the positive edge
      // This can be changed independently
      | (0<<USICS0)
      //Set the data register to clock on external clock pulses
      // (will be generated via software...)
      | (1<<USICS1)
      //Use the software-clock (USITC will toggle the pin, which will
      // in turn increment the counter, and latch data)
      | (1<<USICLK)

      //Strobe the clock (toggle the output pin)
      | (1<<USITC);

   asm("nop;");
}



uint8_t spi_transferByteWithTimer(uint8_t txByte)
{
   USIDR = txByte;

      //This might actually not be safe, because it would also clear bits
      // that are flagged as 1...
      // Also might rewrite the counter value to its old value if it had
      //  been updated during the read->write of USISR
      //  (only a problem when not software-clocked...)
      //clrbit(USIOIF, USISR);
#define spi_clearCounterOverflowFlag_AndCounter()  USISR = (1<<USIOIF)
      spi_clearCounterOverflowFlag_AndCounter();
      
      //Shift the bits until the data completes
      while(!getbit(USIOIF, USISR))
      {
         //Do this first, so we know the first bit will be a full TCNT
         uint8_t tcntStart = TCNT0L;

         while(TCNT0L == tcntStart) asm("nop;");

         spi_StrobeClockAndShift();
      }
      
      //The flag doesn't reset itself...
      spi_clearCounterOverflowFlag_AndCounter();

      uint8_t dataIn = USIDR;

#if (defined(PRINTEVERYBYTE) && PRINTEVERYBYTE)
      sprintf_P(stringBuffer, PSTR("0x%"PRIx8">0x%\n\r"PRIx8));
      puat_sendStringBlocking(stringBuffer);
#endif

      return dataIn;
}


uint8_t spi_transferByte(uint8_t txByte)
{
   USIDR = txByte;
   spi_clearCounterOverflowFlag_AndCounter();

// while(!getbit(USIOIF, USISR))
// {
//    spi_StrobeClockAndShift();
// }

   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();

   spi_clearCounterOverflowFlag_AndCounter();

#if(defined(PRINTEVERYBYTE) && PRINTEVERYBYTE)
   sprintf_P(stringBuffer, PSTR("tx:0x%"PRIx8" -> rx:0x%"PRIx8"\n\r"),
                                       txByte, USIDR);
   puat_sendStringBlocking(stringBuffer);
#endif

   return USIDR;
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
 * /home/meh/_avrProjects/audioThing/55-git/_commonCode_localized/usi_spi/0.10ncf/usi_spi.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
