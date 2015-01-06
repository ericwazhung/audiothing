/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//This is stolen from polled_uat/0.75/testTxRxMega328p/
// And modified, of course.

// The idea: When connected to a computer via the PUART:
// Data sent from the PC will be retransmitted over SPI
// And data received via SPI (which only occurs when data's transmitted)
// will be retransmitted to the computer
// Thus, if the SPI is looped-back (MOSI connected to MISO)
// data transmitted from the computer should be received by the computer
// LIKELY there will be a one or two byte delay:
//  We'll have to look into this.


// Notes are probably not all updated, and probably weren't in "test"...
// (and below is a prime example)



// Prints "Hello World" on boot to the RS-232 Transmitter, then echos
// anything received, therafter.
// Send the numbers 0-9 to this device via RS-232.
// On reception, the heart will blink the requested number of times.
// (0 puts it back into fading-mode)



#include _HEARTBEAT_HEADER_
#include _POLLED_UAT_HEADER_
#include _POLLED_UAR_HEADER_
//#include "../usart_spi.h"
#include _USART_SPI_HEADER_


#include <stdio.h> //necessary for sprintf_P...
                  // See notes in the makefile re: AVR_MIN_PRINTF
#include <util/delay.h>

//For large things like this, I prefer to have them located globally (or
//static) in order to show them in the memory-usage when building...
char stringBuffer[80];


int main(void)
{

   init_heartBeat();

   //a/o 0.70, tcnter_init() must be called *before* puat_init()
   tcnter_init();
   puat_init(0);
   puar_init(0);

   spi_init(USART_SPI_FAST_BAUD_REG_VAL);

   //Likewise, sendStringBlocking[_P]() blocks tcnter and other updates
   // so shouldn't be used when e.g. expecting RX-data...
   //But it's OK now, we're still booting.
   puat_sendStringBlocking_P(0, stringBuffer, 
    PSTR("\n\r\n\r"
         "Type A Key (0-9 adjusts Heart).\n\r"));
   puat_sendStringBlocking_P(0, stringBuffer, 
    PSTR("'F' sets Fast SPI transmission (Default).\n\r"));
   puat_sendStringBlocking_P(0, stringBuffer, 
    PSTR("'S' sets Slow SPI transmission.\n\r"));
   puat_sendStringBlocking_P(0, stringBuffer, 
    PSTR("Each keypress will be retransmitted to the SPI port.\n\r"));
   puat_sendStringBlocking_P(0, stringBuffer, 
    PSTR("At the same time a byte will be received via the SPI port\n\r"));
   puat_sendStringBlocking_P(0, stringBuffer, 
    PSTR("and retransmitted again to here.\n\r"));
   puat_sendStringBlocking_P(0, stringBuffer, 
    PSTR(
   "If MOSI is looped-back to MISO, this should appear as an 'echo'\n\r"));
   puat_sendStringBlocking_P(0, stringBuffer, 
    PSTR("There may be a byte-delay.\n\r"));

   setHeartRate(0);


   static dms4day_t startTime = 0;
   uint8_t fastSPI=TRUE;

   while(1)
   {
      tcnter_update();
      puat_update(0);
      puar_update(0);

      if(dmsIsItTime(&startTime, 1*DMS_SEC))
      {
         puat_sendByteBlocking(0, '.');
      }



      if(puar_dataWaiting(0))
      {
         uint8_t byte = puar_getByte(0);

         if((byte >= '0') && (byte <= '9'))
            set_heartBlink(byte-'0');

         if((byte == 'F') || (byte == 'f'))
            fastSPI = TRUE;
         if((byte == 'S') || (byte == 's'))
            fastSPI = FALSE;


         //Retransmit the received character via SPI
         // (and receive a byte via SPI)
         //The output buffer shouldn't be full, right?
         if(fastSPI)
            byte=spi_transferByte(byte);         
         else
            byte=spi_transferByteWithTimer(byte);


         //Retransmit the SPI-Received byte via PUAT:
         //The output buffer shouldn't be full, right?
         if(!puat_dataWaiting(0))
            puat_sendByte(0, byte);

      }


      heartUpdate();


   }

   return 0;
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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/usart_spi/0.10ncf/testMega328p+FastSlow/main.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
