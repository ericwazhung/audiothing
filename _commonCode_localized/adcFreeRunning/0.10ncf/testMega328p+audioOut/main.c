/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//Stolen from polled_uat/0.75/testMega328p/



#include _HEARTBEAT_HEADER_
#include _POLLED_UAT_HEADER_
#include _ADC_FREE_RUNNING_HEADER_

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
   puat_init(0);

   adcFR_init();

   puat_sendStringBlocking_P(0, stringBuffer, 
PSTR("\n\rNOTE: sample-count will likely be smaller than expected due"));

   puat_sendStringBlocking_P(0, stringBuffer, 
PSTR("\n\r      to blocking serial transfers.\n\r"));

   setHeartRate(0);

   //Initialize PWM output on OC2B/PD3:
   timer_init(2, CLKDIV1, WGM_FAST_PWM);
   timer_setOutputModes(2, OUT_B, COM_CLR_ON_COMPARE);
   setoutPORT(PD3, PORTD);

   static dms4day_t startTime = 0;

   //Let's printout the largest ADC-value measured each second...
   int16_t adcMax = INT16_MIN;
   int16_t adcMin = INT16_MAX;
   //Also, let's printout how many samples were received...
   uint32_t sampleCount = 0;

   while(1)
   {
      tcnter_update();
      puat_update(0);

      int16_t adcVal = adcFR_get();

      //adcFR_get() returns non-negative when a new value arrives
      if(adcVal >= 0)
      {
         sampleCount++;

         if(adcVal > adcMax)
            adcMax = adcVal;

         if(adcVal < adcMin)
            adcMin = adcVal;

         //Output the ADC value (in 8-bit) via PWM on OC2B:
         OCR2B = adcVal >> 2;
      }

      if(dmsIsItTime(&startTime, 1*DMS_SEC))
      {
         sprintf_P(stringBuffer, 
          PSTR("samples: %"PRIu32" min: %"PRIi16" max: %"PRIi16"\n\r"), 
               sampleCount, adcMin, adcMax);
   
         puat_sendStringBlocking(0, stringBuffer);

         //puat_sendByteBlocking(0, count);

         //count++;
         //if(count == '9'+1)
         // count = '0';

         adcMax = INT16_MIN;
         adcMin = INT16_MAX;
         sampleCount = 0;
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
 * /home/meh/_avrProjects/audioThing/55-git/_commonCode_localized/adcFreeRunning/0.10ncf/testMega328p+audioOut/main.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
