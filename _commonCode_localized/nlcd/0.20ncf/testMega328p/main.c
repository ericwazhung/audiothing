/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//This is stolen from usart_spi/0.10ncf/testMega328p+FastSlow/
// which was stolen from polled_uat/0.75/testTxRxMega328p/
//And modified, of course.

// The Idea:
//  Upon power-up the display shows "BOOT"
//  Send data to the device via a serial terminal-emulator
//  Data sent will be displayed in a scrolling-manner on the LCD
//  (It will also be immediately echoed back to the serial port, for
//   debugging purposes)
//  NOTE: As this is an *early* test-program, aimed *mostly* at the NLCD...
//   There are several bugs that should be considered...
//   These NLCD functions, mostly, *block*
//   Thus, since we're using a polled-uart NLCD will *definitely* interfere
//   with reception/transmission
//   As it stands, the UART's "echo" functionality is therefore broken
//   Likewise, only *one* byte can be received at a time...
//   e.g. 'echo "asdf" > /dev/ttyUSB0' will only receive 'a'
//
// usart_spi's test (this was derived from):
//  Implemented a serial "echo" function by retransmitting received UART
//  data through SPI, looping-back the SPI MOSI to MISO, then
//  retransmitting the data received by MISO over the serial-port
// SPI-Loopback is *not* used here



#include _HEARTBEAT_HEADER_
#include _POLLED_UAT_HEADER_
#include _POLLED_UAR_HEADER_
//#include "../usart_spi.h"
#include _USART_SPI_HEADER_
#include _NLCD_HEADER_


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

   spi_init(USART_SPI_SLOW_BAUD_REG_VAL);
   
   nlcd_init();
   nlcd_appendCharacter('B');
   nlcd_appendCharacter('o');
   nlcd_appendCharacter('o');
   nlcd_appendCharacter('t');
   nlcd_redrawCharacters();

   //Likewise, sendStringBlocking[_P]() blocks tcnter and other updates
   // so shouldn't be used when e.g. expecting RX-data...
   //But it's OK now, we're still booting.
   puat_sendStringBlocking_P(0, stringBuffer, 
    PSTR("\n\r\n\r"
         "Type A Key\n\r")); // (0-9 adjusts Heart).\n\r"));
// puat_sendStringBlocking_P(0, stringBuffer, 
//  PSTR("'F' sets Fast SPI transmission.\n\r"));
// puat_sendStringBlocking_P(0, stringBuffer, 
//  PSTR("'S' sets Slow SPI transmission (Default).\n\r"));
   puat_sendStringBlocking_P(0, stringBuffer, 
    PSTR("Each keypress will be displayed on the LCD.\n\r"));
// puat_sendStringBlocking_P(0, stringBuffer, 
//  PSTR(" (and also echoed back to the PC)\n\r"));

   setHeartRate(0);


   static dms4day_t startTime = 0;
// uint8_t fastSPI=FALSE;

   while(1)
   {
      tcnter_update();
      puat_update(0);
      puar_update(0);

      if(nlcd_charactersChanged())
         nlcd_redrawCharacters();


      if(dmsIsItTime(&startTime, 1*DMS_SEC))
      {
         puat_sendByteBlocking(0, '.');
      }



      if(puar_dataWaiting(0))
      {
         uint8_t byte = puar_getByte(0);

         if((byte >= '0') && (byte <= '9'))
            set_heartBlink(byte-'0');

//       if((byte == 'F') || (byte == 'f'))
//          fastSPI = TRUE;
//       if((byte == 'S') || (byte == 's'))
//          fastSPI = FALSE;


         //Retransmit the received character via SPI
         // (and receive a byte via SPI)
         //The output buffer shouldn't be full, right?
//       if(fastSPI)
//          byte=spi_transferByte(byte);         
//       else
//          byte=spi_transferByteWithTimer(byte);

         //NO!!!
         // nlcd uses spi_transferByteWithTimer() (and spi_transferByte()?)
         // On its own... Currently we can't control the NLCD baud-rate in
         // real-time. The rate[s?] used can be set in the makefile.


         nlcd_appendCharacter(byte);



         //Echo the Received byte via PUAT:
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
 * /home/meh/_avrProjects/audioThing/57-heart2/_commonCode_localized/nlcd/0.20ncf/testMega328p/main.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
