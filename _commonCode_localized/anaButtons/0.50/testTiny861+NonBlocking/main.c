/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//This is nearly identical to anaButtons/0.50/test/
// but for the tiny861
// Notes are probably not all updated, and probably weren't in "test"...
// (and below is a prime example)



// Prints "Hello World" on boot to the RS-232 Transmitter, then echos
// anything received, therafter.
// Send the numbers 0-9 to this device via RS-232.
// On reception, the heart will blink the requested number of times.
// (0 puts it back into fading-mode)



#include _ANABUTTONS_HEADER_
#include _HEARTBEAT_HEADER_
#if(!defined(PUAR_DISABLED) || !PUAR_DISABLED)
#include _POLLED_UAR_HEADER_
#endif
#include _POLLED_UAT_HEADER_
#include <stdio.h> //necessary for sprintf_P...
                  // See notes in the makefile re: AVR_MIN_PRINTF
#include <util/delay.h>

//For large things like this, I prefer to have them located globally (or
//static) in order to show them in the memory-usage when building...
char stringBuffer[80];


int main(void)
{

   init_heartBeat();

   tcnter_init();
#if(!defined(PUAR_DISABLED) || !PUAR_DISABLED)
   puar_init(0);
#endif
   puat_init(0);

   // If you're *only* using the tcnter for puat, it's entirely safe to do
   // something like this... BUT, there are various things which may use
   // tcnter in the background without your realizing (puar, for instance)
   // So don't get in the habit of doing this unless you're really on top
   // of things.

   //(Also, should put this in a PSTR() rather than a RAM-based
   //character array...)
/*
   char hello[] = "Hello World\n\r";
   char* character = hello;

   setHeartRate(16);

   //Nothing can be received during this loop...
   while(*character != '\0')
   {
      puat_sendByte(0, *character);
      character++;
      while(puat_dataWaiting(0))
      {
         tcnter_update();
         puat_update(0);
         heartUpdate();
      }
   }
*/
   puat_sendStringBlocking_P(0, stringBuffer, 
                              PSTR("\n\rPress a button on the analog array"
                                    " to see its mesasured value.\n\r"));

   setHeartRate(0);

   while(1)
   {
      tcnter_update();
      puat_update(0);
#if(!defined(PUAR_DISABLED) || !PUAR_DISABLED)
      puar_update(0);
      if(puar_dataWaiting(0))
      {
         uint8_t byte = puar_getByte(0);

         if((byte >= '0') && (byte <= '9'))
            set_heartBlink(byte-'0');

         //Echo the received character...

         //The output buffer shouldn't be full, right?
         if(!puat_dataWaiting(0))
            puat_sendByte(0, byte);
         
      }
#endif

      extern hfm_t heartModulator;
//    static uint8_t lastPower = 0;

//    if(heartModulator.power != lastPower)
//    {
//       lastPower = heartModulator.power;
         //Attempt to simulate varying times between calls to anaButtons...
//       _delay_ms(lastPower);
//    }


/* And, apparently it's not compatible with avr-gcc4.8 and/or avr-libc
 * whatever I'm at...
      //This block is only for the sake of introducing some randomness
      // regarding the length of time each loop takes...
      //E.G. in a real program, some update functions may be quite slow
      //periodically... does that interfere with our anaButtons
      //measurement? Let's find out.
      static dms4day_t startTime = 0;

      if(dmsIsItTime(&startTime, 1*DMS_SEC))
      {
         _delay_ms(heartModulator.power);
      }
      //End of randomness-block.
*/


      heartUpdate();



      int32_t buttonTimeVal = anaButtons_getDebounced();

      if(buttonTimeVal >= 0)
      {
         //char stringBuffer[20];

         extern uint16_t anaB_minSamples;
         extern uint32_t anaB_measurementCount;

         sprintf_P(stringBuffer,
               PSTR("buttonTime=%"PRIi32
                    " sampleCount=%"PRIu16
                    " measurementCount=%"PRIu32"\n\r"), 
                  buttonTimeVal, anaB_minSamples, anaB_measurementCount);
         puat_sendStringBlocking(0, stringBuffer);
      }
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
 * /home/meh/_avrProjects/audioThing/57-heart2/_commonCode_localized/anaButtons/0.50/testTiny861+NonBlocking/main.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
